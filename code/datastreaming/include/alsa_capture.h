#pragma once

#include <alsa/asoundlib.h>
#include <cstdint>

// Lydopsamling fra én ALSA-enhed.
//
// Lyden læses som rå 32-bit samples med to kanaler ved 48 kHz.
// Kanalerne ligger skiftevis i arrayet: kanal 1, kanal 2, kanal 1, kanal 2.
// Programmet står selv for at kopiere samples videre til ringbufferen.

// Åbner enheden p_device, fx "hw:Loopback,1,0", og starter optagelsen.
// Kald med &p_pcm, så funktionen kan give forbindelsen tilbage i p_pcm.
// Returnerer 0 ved succes. Ved fejl returneres en negativ fejlkode,
// og p_pcm sættes til nullptr.
int AlsaCapture_Start(snd_pcm_t** pp_pcm, const char* p_device);

// Læser de næste frame_count frames fra forbindelsen p_pcm til p_samples.
// Fx kræver 256 frames plads til 512 samples, fordi vi har to kanaler.
// Returnerer antal læste frames eller en negativ fejlkode.
// Kun de modtagne frames * 2 værdier i arrayet må bruges bagefter.
snd_pcm_sframes_t AlsaCapture_Read(
    snd_pcm_t* p_pcm, std::int32_t* p_samples, snd_pcm_uframes_t frame_count);

// Lukker forbindelsen p_pcm, når optagelsen er færdig.
// Den må ikke bruges igen, før en ny forbindelse er åbnet.
void AlsaCapture_Stop(snd_pcm_t* p_pcm);
