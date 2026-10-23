#include "../include/alsa_capture.h"

int AlsaCapture_Start(snd_pcm_t** pp_pcm, const char* p_device)
{
    // Åbn enheden og gem forbindelsen i den variabel, programmet har givet os.
    *pp_pcm = nullptr;
    int result = snd_pcm_open(pp_pcm, p_device, SND_PCM_STREAM_CAPTURE, 0);

    if (result < 0) {
        *pp_pcm = nullptr;
        return result;
    }

    // Sæt lydformatet og anmod om 100 ms bufferlatens.
    // ALSA skal ikke omregne samplingfrekvensen i software.
    result = snd_pcm_set_params(
        *pp_pcm,
        SND_PCM_FORMAT_S32_LE,
        SND_PCM_ACCESS_RW_INTERLEAVED,
        2, 48000, 0, 100000);

    if (result >= 0) {
        // Start optagelsen her, så små læsninger ikke skal starte den automatisk.
        result = snd_pcm_start(*pp_pcm);
    }

    // Luk forbindelsen igen, hvis opsætning eller start fejler.
    if (result < 0) {
        snd_pcm_close(*pp_pcm);
        *pp_pcm = nullptr;
    }

    return result;
}

snd_pcm_sframes_t AlsaCapture_Read(
    snd_pcm_t* p_pcm, std::int32_t* p_samples, snd_pcm_uframes_t frame_count)
{
    // Kopier næste blok til arrayet. ALSA holder selv styr på læsepositionen.
    return snd_pcm_readi(p_pcm, p_samples, frame_count);
}

void AlsaCapture_Stop(snd_pcm_t* p_pcm)
{
    if (p_pcm != nullptr) {
        snd_pcm_close(p_pcm);
    }
}
