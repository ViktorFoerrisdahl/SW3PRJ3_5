#pragma once

#include <cstdint>

// Fjerner DC-offset fra én kanal med et løbende gennemsnit (1.-ordens filter):
//
//   mean_ny = mean_gammel + (x - mean_gammel) / 2^k
//   y       = x - mean_ny
//
// Gennemsnittet gemmes som fast-komma-tal (acc = mean · 2^k), så der kun
// bruges heltal: én subtraktion, ét skift og én addition pr. sample.
// Med en lang tidskonstant (stort k) flytter et klap på få ms næsten ikke
// gennemsnittet, mens en langsom drift i offset følges.
class DcRemover {
public:
    explicit DcRemover(int k) : k_(k) {}

    // Tager ét sample (allerede skiftet ned til 18 bit) og returnerer det
    // uden DC-offset. Resultatet kan ligge mellem ca. -2^18 og 2^18.
    int32_t process(int32_t x) {
        if (!initialized_) {
            // Start gennemsnittet på det første sample i stedet for 0, så
            // filteret ikke først skal svinge ind fra nul.
            acc_ = static_cast<int64_t>(x) << k_;
            initialized_ = true;
        }
        // acc >> k er det nuværende gennemsnit. Aritmetisk skift af et
        // negativt tal er veldefineret fra C++20.
        acc_ += static_cast<int64_t>(x) - (acc_ >> k_);
        return x - static_cast<int32_t>(acc_ >> k_);
    }

    // Glemmer gennemsnittet, så det startes forfra på næste sample.
    void reset() { initialized_ = false; }

private:
    int k_;
    int64_t acc_ = 0; // accumulator
    bool initialized_ = false;
};
