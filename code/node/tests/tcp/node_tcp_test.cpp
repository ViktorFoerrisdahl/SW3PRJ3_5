// Test af hele kæden på noden, fra fysisk klap til afsendt TCP-besked:
//
//   ALSA (AlsaCapture) -> RingBuffer -> EventDetector -> EventQueue -> NodeTCP -> orchestrator
//
// Kør orchestrator_tcp_test på orchestratoren først, og start så dette program
// med orchestratorens IP-adresse. For hver hændelse detektoren lægger i køen,
// sender NodeTCP den som én JSON-linje til orchestratoren.
//
// Med --fake bruges ALSA og mikrofonerne ikke. I stedet lægges en kunstig
// hændelse i køen hvert 2. sekund. Så kan TCP-forbindelsen testes alene.
//
// Byg og kør på noden fra denne mappe (se Makefile):
//   make
//   ./node_tcp_test <orchestrator-ip> [port] [alsa-enhed] [node-id]
//   ./node_tcp_test 192.168.1.42 5000 hw:0,0 1
//   ./node_tcp_test 192.168.1.42 --fake
//
// PTP bruges ikke. Timestamps kommer fra nodens CLOCK_REALTIME.

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <memory>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

#include "alsa_capture.h"
#include "detector_config.h"
#include "event_detection.h"
#include "event_queue.h"
#include "node_tcp.h"
#include "ring_buffer.h"

namespace {

// ---- ALSA og ringbuffer (samme værdier som event_detection_hw_test) ----
constexpr uint32_t SAMPLE_RATE_HZ = 48000;
constexpr uint32_t PERIOD_FRAMES = 512;
constexpr uint32_t ALSA_PERIODS = 4;
constexpr std::size_t RING_FRAMES = 131072;

// ---- Tærskel (samme som event_detection_hw_test) ----
constexpr int SAMPLE_BITS = 18;
constexpr double SENSITIVITY_DBFS = -26.0;
constexpr double SPL_REF_DB = 94.0;
constexpr double SPL_TARGET_DB = 100.0;

constexpr int DEFAULT_PORT = 5000;
constexpr auto ANCHOR_TIMEOUT = std::chrono::seconds(3);

volatile std::sig_atomic_t g_stop_requested = 0;
void onSigint(int) { g_stop_requested = 1; }

void usage(const char* prog) {
    std::fprintf(stderr,
                 "Brug: %s <orchestrator-ip> [port] [alsa-enhed] [node-id]\n"
                 "      %s <orchestrator-ip> [port] --fake [node-id]\n"
                 "Standard: port %d, alsa-enhed hw:0,0, node-id 1\n",
                 prog, prog, DEFAULT_PORT);
}

int64_t nowNs() {
    timespec ts{};
    clock_gettime(CLOCK_REALTIME, &ts);
    return static_cast<int64_t>(ts.tv_sec) * 1'000'000'000LL + ts.tv_nsec;
}

// Laver en kunstig hændelse: 50 ms stilhed med en kort puls ved triggerpunktet.
DetectedEvent makeFakeEvent(uint16_t node_id, uint32_t seq) {
    DetectedEvent ev;
    ev.node_id = node_id;
    ev.event_seq = seq;
    ev.sample_rate = SAMPLE_RATE_HZ;
    ev.trigger_offset = 720;  // 15 ms ved 48 kHz
    ev.frames.resize(2400);   // 50 ms ved 48 kHz
    for (int k = 0; k < 48; ++k) {
        const int32_t v = ((k % 2) ? 1 : -1) * (50000 << 14);
        ev.frames[ev.trigger_offset + k] = Frame{{v, v / 2}};
    }
    ev.t_first_ns = nowNs() - 15'000'000LL;
    return ev;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    // ---- Argumenter ----
    const std::string ip = argv[1];
    int port = DEFAULT_PORT;
    std::string device = "hw:0,0";
    uint16_t node_id = 1;
    bool fake = false;

    int pos = 0;  // position blandt de valgfrie argumenter
    for (int i = 2; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--fake") {
            fake = true;
            pos = 2;  // næste argument er node-id
        } else if (pos == 0) {
            port = std::atoi(a.c_str());
            pos = 1;
        } else if (pos == 1) {
            device = a;
            pos = 2;
        } else {
            node_id = static_cast<uint16_t>(std::atoi(a.c_str()));
        }
    }

    std::signal(SIGINT, onSigint);

    EventQueue queue(16);

    // ---- Start TCP-modulet. Det forbinder selv igen, hvis forbindelsen tabes ----
    std::unique_ptr<NodeTCP> tcp;
    try {
        tcp = std::make_unique<NodeTCP>(ip, port, queue);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what());
        usage(argv[0]);
        return 1;
    }
    tcp->start();
    std::printf("Node %u sender til %s:%d\n", node_id, ip.c_str(), port);

    // ---- Fake-tilstand: ingen mikrofoner, bare en hændelse hvert 2. sekund ----
    if (fake) {
        std::printf("Fake-tilstand: sender en kunstig hændelse hvert 2. sekund. Ctrl+C for at stoppe.\n\n");
        uint32_t seq = 0;
        while (!g_stop_requested) {
            DetectedEvent ev = makeFakeEvent(node_id, seq);
            std::printf("Lagt i kø: hændelse #%u (%zu frames)\n", seq, ev.frames.size());
            std::fflush(stdout);
            queue.push(std::move(ev));
            ++seq;
            for (int i = 0; i < 20 && !g_stop_requested; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        return 0;
    }

    // ---- Åbn ALSA og start optagelsen ----
    const double threshold = thresholdRaw(SAMPLE_BITS, SENSITIVITY_DBFS, SPL_TARGET_DB, SPL_REF_DB);
    std::printf("Tærskel: %.0f dB SPL -> THRESHOLD_RAW = %.1f (18-bit)\n", SPL_TARGET_DB, threshold);

    RingBuffer ring_buffer(RING_FRAMES, SAMPLE_RATE_HZ);
    AlsaCapture capture(ring_buffer);
    std::string error;
    if (!capture.open({device, SAMPLE_RATE_HZ, PERIOD_FRAMES, ALSA_PERIODS}, error)) {
        std::fprintf(stderr, "Kunne ikke åbne %s: %s\n", device.c_str(), error.c_str());
        return 1;
    }
    std::atomic<bool> capture_failed{false};
    std::jthread capture_thread([&](std::stop_token st) {
        if (!capture.run(st)) capture_failed = true;
    });

    // ---- Vent til ringbufferen kan give timestamps ----
    const auto deadline = std::chrono::steady_clock::now() + ANCHOR_TIMEOUT;
    int64_t t_ns = 0;
    while (!(ring_buffer.writeIndex() > 0 &&
             ring_buffer.timestampNs(ring_buffer.writeIndex() - 1, t_ns))) {
        if (std::chrono::steady_clock::now() > deadline || capture_failed || g_stop_requested) {
            std::fprintf(stderr, "Ringbufferen gav ingen timestamps inden for %lld s\n",
                         static_cast<long long>(ANCHOR_TIMEOUT.count()));
            capture_thread.request_stop();
            capture_thread.join();
            return 1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // ---- Start detektoren. Den lægger hændelser i køen, som NodeTCP sender ----
    DetectorConfig cfg;
    cfg.node_id = node_id;
    cfg.sample_rate = SAMPLE_RATE_HZ;
    cfg.threshold_raw = {threshold, threshold};

    EventDetector detector(ring_buffer, queue, cfg);
    detector.start();

    std::printf("Lytter på %s. Klap for at teste, Ctrl+C for at stoppe.\n\n", device.c_str());
    std::fflush(stdout);

    // Udskriv lokalt hver gang detektoren har lagt en ny hændelse i køen
    uint64_t last_sent = 0;
    while (!g_stop_requested && !capture_failed) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        const uint64_t sent = detector.eventsSent();
        if (sent != last_sent) {
            std::printf("Klap detekteret, lagt i kø til afsendelse (i alt %llu)\n",
                        static_cast<unsigned long long>(sent));
            std::fflush(stdout);
            last_sent = sent;
        }
    }

    // ---- Stop i rækkefølge: detektor, optagelse, TCP ----
    detector.stop();
    capture_thread.request_stop();
    capture_thread.join();
    tcp.reset();

    std::printf("\nOpsummering\n");
    std::printf("  hændelser lagt i kø:     %llu\n", static_cast<unsigned long long>(detector.eventsSent()));
    std::printf("  hændelser kasseret:      %llu\n", static_cast<unsigned long long>(detector.eventsDiscarded()));
    std::printf("  kun én mikrofon over:    %llu\n", static_cast<unsigned long long>(detector.unpairedTriggers()));
    std::printf("  smidt væk (kø fuld):     %llu\n", static_cast<unsigned long long>(queue.dropped()));
    std::printf("  ALSA-overruns:           %llu\n", static_cast<unsigned long long>(capture.overrunCount()));
    return capture_failed ? 1 : 0;
}
