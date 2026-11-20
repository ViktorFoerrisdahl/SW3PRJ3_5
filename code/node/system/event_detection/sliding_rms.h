#pragma once

#include <cstdint>
#include <vector>

// Glidende RMS-vindue for én kanal.
//
// For hvert nyt sample lægges dets kvadrat til summen, og kvadratet af det
// ældste sample i vinduet trækkes fra:
//
//   sum_ny = sum_gammel + x_n^2 - x_(n-N)^2
//
// Så skal hele vinduet ikke summeres igen for hvert sample. Kvadratroden
// undgås ved at sammenligne summen direkte med N · THRESHOLD_RAW^2.
class SlidingRms {
public:
    // window_frames er N, antallet af samples i vinduet.
    explicit SlidingRms(uint32_t window_frames);

    // Lægger et nyt sample ind i vinduet og fjerner det ældste.
    void push(int32_t x);

    // Summen af kvadraterne i vinduet. Med 18-bit samples (efter
    // DC-fjernelse op til ca. 2^18) er hvert kvadrat højst ca. 6,9·10^10,
    // og selv 240 af dem giver ca. 1,6·10^13, langt under int64_t's
    // grænse på ca. 9,2·10^18.
    int64_t sum() const { return sum_; }

    // Sand når vinduet er fyldt første gang efter oprettelse eller reset().
    bool full() const { return count_ >= squares_.size(); }

    // Tømmer vinduet.
    void reset();

private:
    std::vector<int64_t> squares_;  // kvadraterne i vinduet, brugt som ringbuffer
    std::size_t pos_ = 0;           // hvor det ældste kvadrat ligger
    std::size_t count_ = 0;         // antal samples lagt ind siden reset
    int64_t sum_ = 0;
};
