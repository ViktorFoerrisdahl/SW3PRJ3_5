# Hændelsesdetektering (slave)

Detekterer klap på én mikrofonnode og lægger et udsnit af de rå samples omkring klappet i en kø til TCP-modulet. Designet står i "Hændelsesdetektering" i rapporten.

## Filer

| Fil | Indhold |
|---|---|
| `detector_config.h` | Alle parametre (`DetectorConfig`) og formlen for `THRESHOLD_RAW` (`thresholdRaw()`) |
| `dc_remover.h` | Løbende gennemsnit pr. kanal, der fjerner DC-offset |
| `sliding_rms.h/.cpp` | Glidende RMS-vindue med `int64_t`-sum |
| `event_queue.h` | `DetectedEvent` (output) og den trådsikre kø til TCP-modulet |
| `event_detection.h/.cpp` | `EventDetector`: tråden, tilstandsmaskinen og udklip af udsnittet |

Tests og en mock af ringbufferen ligger i `tests/event_detection/`. Byggekommandoen står øverst i testfilen.

## Antagelser om ringbufferen (input)

Ringbufferen laves af et andet gruppemedlem. Indtil den findes, antager vi at `ring_buffer.h` indeholder følgende. Navnene rettes, når den rigtige ringbuffer er skrevet.

```cpp
struct Frame { int32_t ch[2]; };   // ét S32_LE-sample pr. mikrofon

class RingBuffer {
public:
    // Globalt, monotont voksende antal frames skrevet i alt.
    uint64_t writeIndex() const;
    // Venter (condition variable) til writeIndex() >= min_write_index eller
    // der anmodes om stop. Returnerer writeIndex().
    uint64_t waitForFrames(uint64_t min_write_index, std::stop_token st) const;
    // Kopierer count frames fra globalt frame-nummer start. Returnerer false,
    // hvis nogle af dem ikke er skrevet endnu eller er blevet overskrevet.
    bool copyFrames(uint64_t start, std::size_t count, Frame* dst) const;
    // PTP-synkroniseret timestamp i ns for et globalt frame-nummer.
    bool timestampNs(uint64_t frame, int64_t& t_ns) const;
};
```

Det kræver af ringbufferen:
- en mutex og en `std::condition_variable_any`, og at den kalder `notify_all()` efter hver skrivning (efter write-tælleren er opdateret under låsen),
- at write-tælleren tæller frames og aldrig wrapper (indeks i arrayet = tæller % størrelse),
- at den kan give et timestamp for en vilkårlig frame (fx ved interpolation mellem ét timestamp pr. ALSA-period),
- at den er mindst `pre + post + én period + margin` lang (50 ms + ca. 10 ms + margin).

## Output til TCP-modulet

TCP-modulet kalder `EventQueue::pop(stop_token)`, som venter på næste `DetectedEvent`. Hændelsen indeholder node-id, løbenummer, sample rate, antal kanaler, timestamp for udsnittets første frame, triggerpunktets placering i udsnittet og de rå interleaved samples. At pakke det til bytes (header, endianness) er TCP-modulets ansvar. Er køen fuld, smides den ældste hændelse væk, så detektoren aldrig venter på netværket.

## Brug

```cpp
DetectorConfig cfg;
cfg.node_id = 1;
cfg.threshold_raw = {/* mikrofon 0 */, /* mikrofon 1 */};  // besluttes af gruppen
EventQueue queue(cfg.max_queued_events);
EventDetector detector(ringBuffer, queue, cfg);
detector.start();
// ...
detector.stop();
```

Detektoren kaster `std::invalid_argument`, hvis tærsklen ikke er sat, eller hvis `holdoff_ms < post_ms`.

## Stadig åbent
- Tærsklen pr. mikrofon (gruppen beslutter).
- Om de nederste 14 bit faktisk er 0, og hvor stor DC-offset er (måles på opstillingen).
- Pre/post, RMS-vindue og parringsvindue skal efterprøves ved at klappe i det fjerneste hjørne.
