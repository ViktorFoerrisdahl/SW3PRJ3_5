#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

static void Check(bool passed, const std::string &message)
{
    if (!passed)
        throw std::runtime_error(message);
}

static void WriteScript(const fs::path &path, const char *body)
{
    std::ofstream script(path);
    script << "#!/bin/sh\n" << body;
    script.close();
    Check(static_cast<bool>(script), "Could not write " + path.string());
    fs::permissions(path, fs::perms::owner_exec, fs::perm_options::add);
}

static std::string ReadFile(const fs::path &path)
{
    std::ifstream file(path);
    Check(static_cast<bool>(file), "Could not read " + path.string());
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

static int RunNode(const fs::path &directory, const std::string &arguments, const char *failure = "")
{
    fs::remove(directory / "commands.log");
    Check(setenv("PTP_TEST_FAIL", failure, 1) == 0, "Could not set fake command result");
    const std::string command =
        "cd \"$PTP_TEST_DIR\" && \"$PTP_TEST_NODE\" " + arguments +
        " < input.txt > stdout.txt 2> stderr.txt";
    const int result = std::system(command.c_str());
    Check(result != -1 && WIFEXITED(result), "Could not run ptp_node");
    return WEXITSTATUS(result);
}

static void TestStartAndStop(const fs::path &directory)
{
    for (const std::string role : {"grandmaster", "slave"})
    {
        const std::string ptp = "atlas-ptp-" + role + ".service";
        const std::string phc = "atlas-phc-" + role + ".service";
        const std::string commands = "start " + ptp + " " + phc + "\nstop " + phc + " " + ptp + "\n";

        Check(RunNode(directory, role) == 0, role + ": services did not start and stop");
        Check(ReadFile(directory / "stdout.txt").find("PTP services started.") != std::string::npos,
              role + ": missing startup message");
        Check(ReadFile(directory / "commands.log") == commands, role + ": unexpected service commands");

        if (role == "slave")
        {
            Check(RunNode(directory, role, "start") == 1, "Start failure should return 1");
            Check(ReadFile(directory / "stderr.txt").find("Could not start PTP services.") != std::string::npos,
                  "Start failure should be reported on stderr");
            Check(ReadFile(directory / "commands.log") == commands, "Start failure should stop the services");

            Check(RunNode(directory, role, "stop") == 1, "Stop failure should return 1");
            Check(ReadFile(directory / "stderr.txt").find("Could not stop PTP services.") != std::string::npos,
                  "Stop failure should be reported on stderr");
        }
    }

    Check(RunNode(directory, "invalid") == 1, "Invalid role should return 1");
    Check(!fs::exists(directory / "commands.log"), "Invalid role should leave services alone");
}

static void TestStatus(const fs::path &directory)
{
    const std::string locked = "CLOCK_REALTIME phc offset -100 s2\nmaster offset 50 s3";
    const struct STATUS_CASE
    {
        const char *name;
        std::string journal;
        const char *port;
        int exit_code;
    } cases[] = {
        {"locked offsets", locked, "SLAVE", 0},
        {"wrong port state", locked, "LISTENING", 2},
        {"unlocked servo", locked + "\nmaster offset 50 s0", "SLAVE", 2},
        {"offset over budget", locked + "\nmaster offset 200000 s2", "SLAVE", 2},
        {"missing offsets", "", "SLAVE", 2},
    };

    for (const auto &test : cases)
    {
        Check(setenv("PTP_TEST_JOURNAL", test.journal.c_str(), 1) == 0, "Could not set fake journal");
        Check(setenv("PTP_TEST_PORT", test.port, 1) == 0, "Could not set fake port state");
        Check(RunNode(directory, "slave status") == test.exit_code,
              std::string(test.name) + ": unexpected exit code");
        if (test.exit_code == 0)
            Check(ReadFile(directory / "stdout.txt") == "150\n", "Expected an offset budget of 150 ns");
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Expected the path to ptp_node\n";
        return 1;
    }

    auto directory_template = (fs::temp_directory_path() / "ptp-service-test-XXXXXX").string();
    const char *p_directory = mkdtemp(directory_template.data());
    if (!p_directory)
    {
        std::cerr << "Could not create a temporary test directory\n";
        return 1;
    }
    const fs::path directory = p_directory;

    int result = 0;
    try
    {
        WriteScript(directory / "systemctl", R"(printf '%s\n' "$*" >> commands.log
case "$1" in
    show) printf '%s\n' aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa ;;
    is-active) printf '%s\n' active ;;
esac
[ "$1" != "$PTP_TEST_FAIL" ]
)");
        WriteScript(directory / "pmc", "printf 'portState %s\\n' \"$PTP_TEST_PORT\"\n");
        WriteScript(directory / "journalctl", "printf '%s\\n' \"$PTP_TEST_JOURNAL\"\n");
        std::ofstream input(directory / "input.txt");
        input << '\n';
        input.close();
        Check(static_cast<bool>(input), "Could not write ptp_node input");

        // Limit command lookup to the fake commands in this test.
        Check(setenv("PATH", directory.c_str(), 1) == 0 &&
                  setenv("PTP_TEST_DIR", directory.c_str(), 1) == 0 &&
                  setenv("PTP_TEST_NODE", fs::absolute(argv[1]).c_str(), 1) == 0,
              "Could not set up the test environment");
        TestStartAndStop(directory);
        TestStatus(directory);
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        result = 1;
    }

    fs::remove_all(directory);
    return result;
}
