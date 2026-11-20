// Test af OrchestratorTCP: modtager DetectedEvent fra noderne og udskriver dem.
//
//   node_tcp_test (node) --TCP/JSON--> OrchestratorTCP -> EventQueue -> udskrift
//
// Start dette program først, og start så node_tcp_test på noden med
// orchestratorens IP-adresse (find den med: hostname -I).
//
// Byg og kør på orchestratoren fra denne mappe (se Makefile):
//   make
//   ./orchestrator_tcp_test [port]        standard port 5000
//
// Stop med Ctrl+C.
//
// Forsinkelsen der udskrives er orchestratorens ur minus nodens timestamp for
// triggerpunktet. Uden PTP er urene kun synkroniseret via NTP, så tallet er
// kun et groft skøn.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <stop_token>
#include <string>
#include <thread>

#include "event_queue.h"
#include "orchestrator_tcp.h"

namespace {

constexpr int DEFAULT_PORT = 5000;

int64_t nowNs() {
    timespec ts{};
    clock_gettime(CLOCK_REALTIME, &ts);
    return static_cast<int64_t>(ts.tv_sec) * 1'000'000'000LL + ts.tv_nsec;
}

// Formaterer et timestamp i ns siden 1970 som lokal tid "HH:MM:SS.mmm".
std::string formatTime(int64_t t_ns) {
    const std::time_t sec = static_cast<std::time_t>(t_ns / 1'000'000'000LL);
    const int ms = static_cast<int>((t_ns / 1'000'000LL) % 1000);
    std::tm tm{};
    localtime_r(&sec, &tm);
    char buf[32];
    std::snprintf(buf, sizeof buf, "%02d:%02d:%02d.%03d", tm.tm_hour, tm.tm_min, tm.tm_sec, ms);
    return buf;
}

void printEvent(const DetectedEvent& ev) {
    // Største absolutte værdi pr. mikrofon, i 18-bit-værdier (samme skift som detektoren)
    int32_t peak[2] = {0, 0};
    for (const Frame& f : ev.frames) {
        for (int c = 0; c < 2; ++c) {
            peak[c] = std::max(peak[c], std::abs(f.ch[c] >> 14));
        }
    }

    const double fs = ev.sample_rate > 0 ? ev.sample_rate : 1.0;
    const int64_t t_trigger_ns =
        ev.t_first_ns + static_cast<int64_t>(std::llround(ev.trigger_offset * 1e9 / fs));
    const double delay_ms = (nowNs() - t_trigger_ns) / 1e6;

    std::printf("Modtaget fra node %u: hændelse #%u\n", ev.node_id, ev.event_seq);
    std::printf("  sample_rate %u Hz, %zu frames (%.1f ms), trigger_offset %u\n",
                ev.sample_rate, ev.frames.size(), ev.frames.size() * 1e3 / fs, ev.trigger_offset);
    std::printf("  t_first_ns %lld, trigger kl. %s, forsinkelse ca. %.1f ms\n",
                static_cast<long long>(ev.t_first_ns), formatTime(t_trigger_ns).c_str(), delay_ms);
    std::printf("  peak mic0 %d, peak mic1 %d (18-bit)\n", peak[0], peak[1]);
    if (!ev.frames.empty()) {
        const Frame& t = ev.frames[std::min<std::size_t>(ev.trigger_offset, ev.frames.size() - 1)];
        std::printf("  frame ved trigger: ch0 %d, ch1 %d (rå S32)\n", t.ch[0], t.ch[1]);
    }
    std::printf("\n");
    std::fflush(stdout);
}

}  // namespace

int main(int argc, char** argv) {
    const int port = (argc > 1) ? std::atoi(argv[1]) : DEFAULT_PORT;

    EventQueue queue(64);

    // Tråd der henter hændelser ud af køen og udskriver dem
    std::jthread printer([&](std::stop_token st) {
        while (auto ev = queue.pop(st)) {
            printEvent(*ev);
        }
    });

    try {
        OrchestratorTCP tcp(port, queue);
        std::printf("Orchestrator lytter på port %d. Ctrl+C for at stoppe.\n\n", port);
        std::fflush(stdout);
        tcp.start();  // venter på forbindelser fra noderne, returnerer ikke
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Fejl: %s (er porten allerede i brug?)\n", e.what());
        printer.request_stop();
        return 1;
    }
    return 0;
}
