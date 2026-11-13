// Tests af hændelsesdetekteringen med en mock-ringbuffer og syntetiske klap.
//
// Byg og kør fra code/node:
//   g++ -std=c++23 -Wall -Wextra -O2 -pthread -I tests/event_detection/mocks -I src/event_detection src/event_detection/event_detection.cpp src/event_detection/sliding_rms.cpp tests/event_detection/test_event_detection.cpp -o test_event_detection
//   ./test_event_detection
//
// Tærsklen i testene (TEST_THRESHOLD) er en vilkårlig testværdi, valgt så de
// syntetiske klap ligger tydeligt over og støjen tydeligt under. Den er ikke
// et forslag til den rigtige tærskel.

#include <chrono>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <thread>
#include <vector>

#include "event_detection.h"
#include "ring_buffer.h"

namespace {

int failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::printf("  FEJL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
            ++failures;                                                      \
        }                                                                    \
    } while (0)

constexpr uint32_t FS = 48000;
constexpr int64_t T0_NS = 1'700'000'000'000'000'000LL;  // vilkårligt starttidspunkt
constexpr double TEST_THRESHOLD = 2000.0;               // kun til test, se øverst

// Et klap i testsignalet: starter i frame start[c] på kanal c.
struct Clap {
    uint64_t start[2];
    double amplitude = 30000.0;  // i 18-bit-værdier
};

// Simpel deterministisk støj, så testene giver samme resultat hver gang.
struct Lcg {
    uint32_t s = 12345;
    int32_t next(int32_t amp) {
        s = s * 1664525u + 1013904223u;
        return static_cast<int32_t>(s >> 8) % (2 * amp + 1) - amp;
    }
};

// Laver et signal i S32_LE-format: 18-bit værdier lagt øverst i 32 bit.
// Hvert klap er en 2 kHz sinus med eksponentielt henfald (tidskonstant 2 ms).
std::vector<Frame> makeSignal(std::size_t frames, const std::vector<Clap>& claps,
                              int32_t dc_offset = 0, int32_t noise = 100) {
    std::vector<Frame> out(frames);
    Lcg rng;
    for (std::size_t i = 0; i < frames; ++i) {
        for (int c = 0; c < 2; ++c) {
            double v = dc_offset + rng.next(noise);
            for (const Clap& clap : claps) {
                if (i >= clap.start[c]) {
                    const double t = static_cast<double>(i - clap.start[c]) / FS;
                    v += clap.amplitude * std::exp(-t / 0.002) * std::sin(2 * M_PI * 2000.0 * t);
                }
            }
            const auto v18 = static_cast<int32_t>(std::lround(v));
            out[i].ch[c] = static_cast<int32_t>(static_cast<uint32_t>(v18) << 14);
        }
    }
    return out;
}

DetectorConfig testConfig() {
    DetectorConfig cfg;
    cfg.node_id = 1;
    cfg.threshold_raw = {TEST_THRESHOLD, TEST_THRESHOLD};
    return cfg;
}

// Skriver signalet til ringbufferen i bidder af 480 frames (10 ms, ca. én
// ALSA-period) og lader detektoren behandle hver bid, som i drift.
void feed(RingBuffer& rb, EventDetector& det, const std::vector<Frame>& sig, std::size_t period = 480) {
    for (std::size_t i = 0; i < sig.size(); i += period) {
        const std::size_t n = std::min(period, sig.size() - i);
        rb.write(sig.data() + i, n);
        det.processAvailable();
    }
}

std::vector<DetectedEvent> drain(EventQueue& q) {
    std::vector<DetectedEvent> out;
    while (auto ev = q.tryPop()) {
        out.push_back(std::move(*ev));
    }
    return out;
}

void testThresholdFormula() {
    std::puts("tærskelformel");
    // Ved SPL_target = SPL_ref er THRESHOLD_RAW = (2^17 - 1) · 10^(-26/20).
    const double expected = 131071.0 * std::pow(10.0, -26.0 / 20.0);
    CHECK(std::abs(thresholdRaw(18, -26.0, 94.0, 94.0) - expected) < 1e-6);
    // 20 dB højere giver 10 gange så stor amplitude.
    CHECK(std::abs(thresholdRaw(18, -26.0, 114.0, 94.0) - 10.0 * expected) < 1e-6);
}

void testSlidingRms() {
    std::puts("glidende RMS-vindue");
    SlidingRms rms(4);
    for (int x : {1, 2, 3, 4}) rms.push(x);
    CHECK(rms.full());
    CHECK(rms.sum() == 1 + 4 + 9 + 16);
    rms.push(10);  // 1 falder ud
    CHECK(rms.sum() == 4 + 9 + 16 + 100);
    rms.push(-131072);  // største negative 18-bit værdi
    CHECK(rms.sum() == 9 + 16 + 100 + 131072LL * 131072LL);
}

void testDcRemover() {
    std::puts("DC-fjernelse");
    DcRemover dc(10);
    CHECK(dc.process(5000) == 0);  // starter på første sample
    int32_t y = 0;
    for (int i = 0; i < 20000; ++i) y = dc.process(5000);
    CHECK(y == 0);
    // Et spring i offset forsvinder igen efter nogle tidskonstanter.
    for (int i = 0; i < 20000; ++i) y = dc.process(8000);
    CHECK(std::abs(y) <= 1);
}

void testSingleClap() {
    std::puts("ét klap på begge mikrofoner");
    RingBuffer rb(2 * FS, FS, T0_NS);
    EventQueue q(16);
    EventDetector det(rb, q, testConfig());

    // Klappet når mikrofon 1 4 ms efter mikrofon 0.
    const auto sig = makeSignal(FS, {Clap{{24000, 24192}}});
    feed(rb, det, sig);

    const auto events = drain(q);
    CHECK(events.size() == 1);
    if (events.size() != 1) return;
    const DetectedEvent& ev = events[0];
    CHECK(ev.node_id == 1);
    CHECK(ev.event_seq == 0);
    CHECK(ev.sample_rate == FS);
    CHECK(ev.trigger_offset == 720);   // 15 ms
    CHECK(ev.frames.size() == 2400);   // 50 ms

    // Udsnittets første frame og dets timestamp.
    const int64_t first = (ev.t_first_ns - T0_NS) * FS / 1'000'000'000LL;
    const uint64_t trigger = static_cast<uint64_t>(first) + ev.trigger_offset;
    // Triggerpunktet ligger efter klappets start og inden RMS-vinduet (3 ms) er gået.
    CHECK(trigger >= 24000 && trigger <= 24000 + 144);
    // Begge mikrofoners klap-start ligger inde i udsnittet.
    CHECK(static_cast<uint64_t>(first) <= 24000 && 24192 < first + ev.frames.size());
    // Udsnittet er de rå samples, uændrede.
    bool same = true;
    for (std::size_t i = 0; i < ev.frames.size(); ++i) {
        const Frame& a = ev.frames[i];
        const Frame& b = sig[first + i];
        same = same && a.ch[0] == b.ch[0] && a.ch[1] == b.ch[1];
    }
    CHECK(same);
}

void testOnlyOneMicrophone() {
    std::puts("kun én mikrofon over tærsklen");
    RingBuffer rb(2 * FS, FS, T0_NS);
    EventQueue q(16);
    EventDetector det(rb, q, testConfig());

    Clap clap{{24000, 0}};
    auto sig = makeSignal(FS, {clap});
    // Fjern klappet fra kanal 1 igen.
    const auto quiet = makeSignal(FS, {});
    for (std::size_t i = 0; i < sig.size(); ++i) sig[i].ch[1] = quiet[i].ch[1];
    feed(rb, det, sig);

    CHECK(drain(q).empty());
    CHECK(det.unpairedTriggers() >= 1);
}

void testDelayTooLarge() {
    std::puts("for stor forsinkelse mellem mikrofonerne");
    RingBuffer rb(2 * FS, FS, T0_NS);
    EventQueue q(16);
    EventDetector det(rb, q, testConfig());

    // 15 ms mellem mikrofonerne, mere end pair_window_ms (10 ms).
    feed(rb, det, makeSignal(FS, {Clap{{24000, 24720}}}));
    CHECK(drain(q).empty());
}

void testHoldoff() {
    std::puts("holdoff");
    {
        // Andet "klap" (ekko) 20 ms efter det første: kun én hændelse.
        RingBuffer rb(2 * FS, FS, T0_NS);
        EventQueue q(16);
        EventDetector det(rb, q, testConfig());
        feed(rb, det, makeSignal(FS, {Clap{{24000, 24100}}, Clap{{24960, 25060}}}));
        CHECK(drain(q).size() == 1);
    }
    {
        // To klap 300 ms fra hinanden: to hændelser med løbenummer 0 og 1.
        RingBuffer rb(2 * FS, FS, T0_NS);
        EventQueue q(16);
        EventDetector det(rb, q, testConfig());
        feed(rb, det, makeSignal(FS, {Clap{{12000, 12100}}, Clap{{26400, 26500}}}));
        const auto events = drain(q);
        CHECK(events.size() == 2);
        if (events.size() == 2) {
            CHECK(events[0].event_seq == 0);
            CHECK(events[1].event_seq == 1);
        }
    }
}

void testDcOffset() {
    std::puts("stor DC-offset");
    RingBuffer rb(2 * FS, FS, T0_NS);
    EventQueue q(16);
    EventDetector det(rb, q, testConfig());

    // Offset på 50000 ville alene ligge langt over tærsklen uden DC-fjernelse.
    // Uden klap: ingen hændelse. Med klap: én hændelse.
    feed(rb, det, makeSignal(FS, {}, 50000));
    CHECK(drain(q).empty());
    feed(rb, det, makeSignal(FS, {Clap{{24000, 24192}}}, 50000));
    CHECK(drain(q).size() == 1);
}

void testOverrun() {
    std::puts("detektoren bliver overhalet af skriveren");
    RingBuffer rb(FS / 10, FS, T0_NS);  // kun 100 ms buffer
    EventQueue q(16);
    EventDetector det(rb, q, testConfig());

    // Skriv 1 s uden at detektoren læser, så den skal starte forfra.
    const auto sig = makeSignal(FS, {});
    rb.write(sig.data(), sig.size());
    det.processAvailable();
    CHECK(det.resyncs() == 1);
    // Herefter fungerer den igen.
    feed(rb, det, makeSignal(FS / 2, {Clap{{12000, 12192}}}));
    CHECK(drain(q).size() == 1);
}

void testThread() {
    std::puts("detektortråd med condition variable");
    RingBuffer rb(2 * FS, FS, T0_NS);
    EventQueue q(16);
    EventDetector det(rb, q, testConfig());
    det.start();

    const auto sig = makeSignal(FS, {Clap{{24000, 24192}}});
    for (std::size_t i = 0; i < sig.size(); i += 480) {
        rb.write(sig.data() + i, std::min<std::size_t>(480, sig.size() - i));
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
    // Vent på hændelsen, som TCP-modulet ville gøre.
    std::stop_source timeout;
    std::jthread timer([&] {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        timeout.request_stop();
    });
    const auto ev = q.pop(timeout.get_token());
    timer.request_stop();
    CHECK(ev.has_value());
    det.stop();  // må ikke hænge
}

void testInvalidConfig() {
    std::puts("ugyldig konfiguration");
    RingBuffer rb(FS, FS, T0_NS);
    EventQueue q(16);
    bool threw = false;
    try {
        EventDetector det(rb, q, DetectorConfig{});  // tærskel ikke sat
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);

    threw = false;
    DetectorConfig cfg = testConfig();
    cfg.holdoff_ms = 10.0;  // kortere end post_ms
    try {
        EventDetector det(rb, q, cfg);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
}

}  // namespace

int main() {
    testThresholdFormula();
    testSlidingRms();
    testDcRemover();
    testSingleClap();
    testOnlyOneMicrophone();
    testDelayTooLarge();
    testHoldoff();
    testDcOffset();
    testOverrun();
    testThread();
    testInvalidConfig();

    if (failures == 0) {
        std::puts("Alle tests bestået.");
        return 0;
    }
    std::printf("%d fejl.\n", failures);
    return 1;
}
