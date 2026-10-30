// Integrationstest af hændelsesdetekteringen på den rigtige hardware.
//
// Kører hele kæden på én node med de rigtige mikrofoner:
//
//   ALSA (AlsaCapture) -> RingBuffer -> EventDetector -> EventQueue -> udskrift
//
// For hvert klap detektoren melder, udskrives løbenummer, tidspunkt, triggerpunktets
// placering i udsnittet, peak pr. mikrofon, og hvor mange ms mikrofon 1 hørte klappet
// efter mikrofon 0 (fundet med krydskorrelation på udsnittet). Programmet kører, til
// det stoppes med Ctrl+C, og udskriver så en opsummering.
//
// Byg og kør på Pi'en fra denne mappe:
//   make
//   make run                  # eller: ./event_detection_hw_test hw:0,0
//   arecord -l                # viser kort- og enhedsnummer, hvis hw:0,0 ikke passer
//
// Kræver libasound2-dev:  sudo apt install libasound2-dev

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

#include "alsa_capture.h"
#include "detector_config.h"
#include "event_detection.h"
#include "event_queue.h"
#include "frame.h"
#include "ring_buffer.h"

namespace {

// ---- ALSA og ringbuffer (samme værdier som ringbufferens alsa_hw_test) ----
constexpr uint32_t SAMPLE_RATE_HZ = 48000;
constexpr uint32_t PERIOD_FRAMES = 512;     // ca. 10,7 ms pr. ALSA-period
constexpr uint32_t ALSA_PERIODS = 4;
constexpr std::size_t RING_FRAMES = 131072; // ca. 2,7 s ved 48 kHz

// ---- Tærskel: gruppen har besluttet at detektere alt over 75 dB SPL ----
// Omregnes til rå PCM med formlen fra rapporten:
//   THRESHOLD_RAW = (2^(N-1) - 1) · 10^((S_dBFS + (SPL_target - SPL_ref)) / 20)
constexpr int SAMPLE_BITS = 18;            // N: samples skiftes ned til 18 bit
constexpr double SENSITIVITY_DBFS = -26.0; // SPH0645LM4H-B, typisk værdi (±3 dB)
constexpr double SPL_REF_DB = 94.0;        // lydtrykket sensitiviteten er målt ved
constexpr double SPL_TARGET_DB = 100.0;     // gruppens tærskel

constexpr uint16_t NODE_ID = 1;

// Største afstand mellem nodens to mikrofoner (nabohjørner) og lydens hastighed.
// Bruges til at begrænse krydskorrelationen til fysisk mulige forsinkelser.
constexpr double MIC_DISTANCE_M = 3.0;
constexpr double SPEED_OF_SOUND_M_S = 343.0;
constexpr double LAG_MARGIN_S = 0.002;

// Hvor længe der højst ventes på, at ringbufferen kan give timestamps.
constexpr auto ANCHOR_TIMEOUT = std::chrono::seconds(3);

// Sættes af Ctrl+C. Et signal-handler må kun skrive til en variabel af denne type.
volatile std::sig_atomic_t g_stop_requested = 0;

void onSigint(int) { g_stop_requested = 1; }

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

// Pakker én kanal ud af udsnittet, skifter til 18 bit og fjerner gennemsnittet (DC).
std::vector<double> extractChannel(const DetectedEvent& ev, int ch) {
    std::vector<double> x(ev.frames.size());
    double sum = 0.0;
    for (std::size_t k = 0; k < x.size(); ++k) {
        x[k] = static_cast<double>(ev.frames[k].ch[ch] >> 14);
        sum += x[k];
    }
    const double mean = sum / static_cast<double>(x.size());
    for (double& v : x) v -= mean;
    return x;
}

// Største absolutte værdi i en kanal, i 18-bit-værdier.
double peakAbs(const std::vector<double>& x) {
    double peak = 0.0;
    for (double v : x) peak = std::max(peak, std::abs(v));
    return peak;
}

// Finder det lag l (i samples) i [-max_lag, max_lag], der giver den største
// krydskorrelation r[l] = sum x0[k] · x1[k + l]. Positivt l betyder, at
// mikrofon 1 hørte klappet efter mikrofon 0.
int bestLag(const std::vector<double>& x0, const std::vector<double>& x1, int max_lag) {
    const int n = static_cast<int>(x0.size());
    int best = 0;
    double best_r = -1e300;
    for (int l = -max_lag; l <= max_lag; ++l) {
        const int k0 = std::max(0, -l);
        const int k1 = std::min(n, n - l);
        double r = 0.0;
        for (int k = k0; k < k1; ++k) r += x0[k] * x1[k + l];
        if (r > best_r) {
            best_r = r;
            best = l;
        }
    }
    return best;
}

// Udskriver én hændelse.
void printEvent(const DetectedEvent& ev) {
    const double fs = ev.sample_rate;
    const int64_t t_trigger_ns =
        ev.t_first_ns + static_cast<int64_t>(std::llround(ev.trigger_offset * 1e9 / fs));

    const auto x0 = extractChannel(ev, 0);
    const auto x1 = extractChannel(ev, 1);

    // Peak i dBFS: 20 · log10(peak / fuld skala), fuld skala = 2^17 - 1 for 18 bit
    const double full_scale = std::pow(2.0, SAMPLE_BITS - 1) - 1.0;
    const double peak0 = peakAbs(x0);
    const double peak1 = peakAbs(x1);
    const double peak0_dbfs = 20.0 * std::log10(std::max(peak0, 1.0) / full_scale);
    const double peak1_dbfs = 20.0 * std::log10(std::max(peak1, 1.0) / full_scale);

    const int max_lag = static_cast<int>(
        std::ceil((MIC_DISTANCE_M / SPEED_OF_SOUND_M_S + LAG_MARGIN_S) * fs));
    const int lag = bestLag(x0, x1, max_lag);
    const double delay_ms = lag / fs * 1e3;
    const double path_diff_cm = lag / fs * SPEED_OF_SOUND_M_S * 100.0;

    std::printf("Hændelse #%u  trigger %s  (t_first_ns=%lld, trigger_offset=%u, %zu frames)\n",
                ev.event_seq, formatTime(t_trigger_ns).c_str(),
                static_cast<long long>(ev.t_first_ns), ev.trigger_offset, ev.frames.size());
    std::printf("  peak mic0 %7.0f (%6.1f dBFS)   peak mic1 %7.0f (%6.1f dBFS)\n",
                peak0, peak0_dbfs, peak1, peak1_dbfs);
    std::printf("  mic1 efter mic0: %+7.3f ms (%+d samples, %+6.1f cm længere lydvej)\n",
                delay_ms, lag, path_diff_cm);
    std::fflush(stdout);
}

}  // namespace

int main(int argc, char** argv) {
    const std::string device = (argc > 1) ? argv[1] : "hw:0,0";
    std::signal(SIGINT, onSigint);

    // ---- Tærskel ----
    const double threshold = thresholdRaw(SAMPLE_BITS, SENSITIVITY_DBFS, SPL_TARGET_DB, SPL_REF_DB);
    std::printf("Tærskel: %.0f dB SPL -> THRESHOLD_RAW = %.1f (18-bit), samme for begge mikrofoner\n",
                SPL_TARGET_DB, threshold);

    // ---- Åbn ALSA og start optagelsen ----
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

    // ---- Vent til ringbufferen kan give timestamps (mindst 8 tidsankre) ----
    // Ellers kasserer detektoren de første hændelser, fordi de mangler et timestamp.
    const auto deadline = std::chrono::steady_clock::now() + ANCHOR_TIMEOUT;
    int64_t t_ns = 0;
    while (!(ring_buffer.writeIndex() > 0 &&
             ring_buffer.timestampNs(ring_buffer.writeIndex() - 1, t_ns))) {
        if (std::chrono::steady_clock::now() > deadline || capture_failed || g_stop_requested) {
            std::fprintf(stderr, "Ringbufferen gav ingen timestamps inden for %lld s\n",
                         static_cast<long long>(ANCHOR_TIMEOUT.count()));
            return 1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // ---- Start detektoren og en tråd, der henter hændelser ud af køen ----
    DetectorConfig cfg;
    cfg.node_id = NODE_ID;
    cfg.sample_rate = SAMPLE_RATE_HZ;
    cfg.threshold_raw = {threshold, threshold};

    EventQueue queue(cfg.max_queued_events);
    EventDetector detector(ring_buffer, queue, cfg);
    detector.start();

    // Står i stedet for TCP-modulet: henter hver hændelse og udskriver den.
    std::jthread consumer_thread([&](std::stop_token st) {
        while (auto ev = queue.pop(st)) {
            printEvent(*ev);
        }
    });

    std::printf("Lytter på %s. Klap for at teste, Ctrl+C for at stoppe.\n\n", device.c_str());
    std::fflush(stdout);

    while (!g_stop_requested && !capture_failed) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // ---- Stop i rækkefølge: detektor, forbruger, optagelse ----
    detector.stop();
    consumer_thread.request_stop();
    consumer_thread.join();
    capture_thread.request_stop();
    capture_thread.join();

    if (capture_failed) {
        std::fprintf(stderr, "\nOptagelsen stoppede med en fejl.\n");
    }
    std::printf("\nOpsummering\n");
    std::printf("  hændelser sendt:         %llu\n", static_cast<unsigned long long>(detector.eventsSent()));
    std::printf("  hændelser kasseret:      %llu  (udsnit overskrevet eller uden timestamp)\n",
                static_cast<unsigned long long>(detector.eventsDiscarded()));
    std::printf("  kun én mikrofon over:    %llu\n", static_cast<unsigned long long>(detector.unpairedTriggers()));
    std::printf("  resyncs:                 %llu\n", static_cast<unsigned long long>(detector.resyncs()));
    std::printf("  smidt væk (kø fuld):     %llu\n", static_cast<unsigned long long>(queue.dropped()));
    std::printf("  ALSA-overruns:           %llu\n", static_cast<unsigned long long>(capture.overrunCount()));
    return capture_failed ? 1 : 0;
}
