#include "../include/alsa_capture.h"
#include "../include/ringbuffer.h"

#include <iostream>

// Afprøver forbindelsen mellem ALSA og vores ringbuffer.
// Lyden hentes i blokke, og de seneste to sekunder bliver liggende i bufferen.
// Testlyden skal køre på loopback med S32_LE, to kanaler og 48 kHz.
int main()
{
    // Plads til to sekunder: 48.000 frames/s * 2 kanaler * 2 sekunder.
    RingBuffer ring_buffer(192000);
    std::int32_t samples[256 * 2]; // plads til én blok fra begge kanaler
    snd_pcm_t* p_pcm = nullptr;   // forbindelsen til ALSA

    int result = AlsaCapture_Start(&p_pcm, "hw:Loopback,1,0");
    if (result < 0) {
        std::cerr << "Start: " << snd_strerror(result) << '\n';
        return 1;
    }

    // 1000 fulde blokke giver ca. 5,33 sekunder, så bufferen når at gå rundt.
    for (int block = 0; block < 1000; ++block) {
        snd_pcm_sframes_t frames = AlsaCapture_Read(p_pcm, samples, 256);

        if (frames < 0) {
            std::cerr << "Read: " << snd_strerror(static_cast<int>(frames)) << '\n';
            AlsaCapture_Stop(p_pcm);
            return 1;
        }

        // Gem begge kanaler i rækkefølge. Når bufferen er fuld,
        // overskriver den selv de ældste samples.
        for (snd_pcm_sframes_t i = 0; i < frames * 2; ++i) {
            ring_buffer.RingBuffer_Write(samples[i]);
        }
    }

    AlsaCapture_Stop(p_pcm);

    const std::size_t stored_count = ring_buffer.RingBuffer_GetSize();
    std::cout << "Stored samples: " << stored_count
              << " | Seconds: " << stored_count / (48000.0 * 2) << '\n';

    // Vis den ældste frame, der stadig ligger i ringbufferen.
    int channel_1 = 0;
    int channel_2 = 0;
    if (ring_buffer.RingBuffer_Read(0, channel_1)
        && ring_buffer.RingBuffer_Read(1, channel_2)) {
        std::cout << "Oldest frame: " << channel_1 << ", " << channel_2 << '\n';
    }

    return 0;
}
