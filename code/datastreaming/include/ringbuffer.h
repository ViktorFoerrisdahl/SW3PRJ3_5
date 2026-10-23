#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// Ringbufferen gemmer de seneste værdier.
// Når den er fuld, overskriver nye værdier de ældste.
// Værdierne bliver ikke fjernet, når vi læser dem.
class RingBuffer
{
public:
    // Opretter bufferen med et bestemt antal pladser.
    // capacity skal være større end 0.
    explicit RingBuffer(std::size_t capacity);

    // Tilføjer en værdi på indeks write_count % capacity.
    // Derefter øges write_count med 1.
    void RingBuffer_Write(int value);

    // Læser fra den ældste værdi og frem: offset 0 er den ældste.
    // Resultatet gemmes i value. Ved ugyldigt offset returneres false,
    // og value ændres ikke.
    bool RingBuffer_Read(std::size_t offset, int& value) const;

    // Giver antallet af værdier, der faktisk er gemt.
    // Det kan højst være capacity.
    std::size_t RingBuffer_GetSize() const;

private:
    // Vektoren får sin størrelse i konstruktøren og beholder den.
    std::vector<int> buffer;

    std::size_t capacity; // Antal pladser i bufferen.

    // Tæller alle tilføjelser og nulstilles ikke, når indekset går rundt.
    // Bruger uint64_t så den ikke behøver at blive nulstillet
    std::uint64_t write_count = 0;
};