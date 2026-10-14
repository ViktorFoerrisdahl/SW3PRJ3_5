#pragma once

#include <array>
#include <cmath>
#include <cstdint>

// Alle parametre til hændelsesdetekteringen samlet ét sted, så de kan
// justeres uden at ændre selve detektoren.
//
// Tidsparametre angives i millisekunder og omregnes til frames med
// msToFrames(). Én frame = ét sample fra hver af nodens to mikrofoner.
struct DetectorConfig {
    // Hvilken mikrofonnode (slave) detektoren kører på. Sendes med i hver hændelse.
    uint16_t node_id = 0;

    // Sample rate i Hz. Vi sampler med 48 kHz i ALSA-formatet S32_LE.
    uint32_t sample_rate = 48000;

    // Antal bits et råt S32_LE-sample skiftes til højre, før det bruges i
    // detektionen. SPH0645LM4H-B har 18 gyldige bits øverst i en 32-bit
    // container, så de nederste 14 bits er altid 0 (skal verificeres på
    // opstillingen).
    int sample_shift = 14;

    // Tærsklen i rå PCM-værdi (efter skift til 18 bit) for hver mikrofon,
    // dvs. THRESHOLD_RAW fra dokumentet. Én værdi pr. mikrofon, fordi
    // sensitiviteten kan variere ±3 dB fra mikrofon til mikrofon.
    // Værdien er IKKE fastsat endnu. Gruppen beslutter den, og den kan
    // udregnes med thresholdRaw() nedenfor. Detektoren nægter at starte,
    // hvis den ikke er sat (> 0).
    std::array<double, 2> threshold_raw{0.0, 0.0};

    // Længden af det glidende RMS-vindue. Dokumentet angiver 2-5 ms
    // (96-240 samples ved 48 kHz).
    double rms_window_ms = 3.0;

    // Hvor lang tid den anden mikrofon har til at overstige tærsklen efter
    // den første: 3 m / 343 m/s ≈ 8,7 ms plus en margin.
    double pair_window_ms = 10.0;

    // Udsnittet der sendes til master: pre_ms før og post_ms efter triggerpunktet.
    double pre_ms = 15.0;
    double post_ms = 35.0;

    // Hvor længe efter et triggerpunkt nye hændelser ignoreres (efterklang,
    // ekko). Skal mindst være post_ms, så samme klap ikke sendes to gange.
    double holdoff_ms = 35.0;

    // Tidskonstant for det løbende gennemsnit, der fjerner DC-offset:
    // mean += (x - mean) >> dc_shift_k. Tidskonstanten er ca. 2^k / fs,
    // så k = 15 giver ca. 0,68 s ved 48 kHz.
    int dc_shift_k = 15;

    // Hvor mange frames detektoren højst kopierer ud af ringbufferen ad gangen.
    uint32_t chunk_frames = 512;

    // Hvor mange færdige hændelser køen til TCP-modulet højst holder.
    // Er køen fuld, smides den ældste hændelse væk.
    uint32_t max_queued_events = 16;

    // Omregner en tid i ms til et antal frames (afrundet til nærmeste heltal).
    uint32_t msToFrames(double ms) const {
        return static_cast<uint32_t>(std::lround(ms * 1e-3 * sample_rate));
    }
};

// Udregner tærsklen i rå PCM-værdi ud fra en tærskel i dB SPL:
//
//   THRESHOLD_RAW = (2^(N-1) - 1) · 10^((S_dBFS + (SPL_target - SPL_ref)) / 20)
//
// bits       N, antal bits i den repræsentation tærsklen bruges på (18 efter skift)
// s_dbfs     mikrofonens sensitivitet i dBFS (typisk -26 for SPH0645LM4H-B)
// spl_target den tærskel i dB SPL gruppen beslutter
// spl_ref    det lydtryk sensitiviteten er målt ved (94 dB SPL)
inline double thresholdRaw(int bits, double s_dbfs, double spl_target, double spl_ref) {
    const double full_scale = std::pow(2.0, bits - 1) - 1.0;
    return full_scale * std::pow(10.0, (s_dbfs + (spl_target - spl_ref)) / 20.0);
}
