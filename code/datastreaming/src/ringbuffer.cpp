#include "../include/ringbuffer.h"

#include <stdexcept>

RingBuffer::RingBuffer(std::size_t capacity)
    : buffer(capacity), capacity(capacity)
{
    if (capacity == 0) {
        throw std::invalid_argument("Capacity must be greater than zero");
    }
}

void RingBuffer::RingBuffer_Write(int value)
{
    // Skriv på næste plads og øg derefter tælleren.
    buffer[write_count % capacity] = value;
    ++write_count;
}

std::size_t RingBuffer::RingBuffer_GetSize() const
{
    // Antallet af gemte værdier kan højst være kapaciteten.
    if (write_count < capacity) {
        return static_cast<std::size_t>(write_count);
    }

    return capacity;
}

bool RingBuffer::RingBuffer_Read(std::size_t offset, int& value) const
{
    const std::size_t stored_count = RingBuffer_GetSize();

    if (offset >= stored_count) {
        return false;
    }

    // Find den ældste gemte værdis fortløbende nummer.
    const std::uint64_t oldest_number = write_count - stored_count;

    // Modulus omsætter nummeret til en plads i vektoren.
    const std::size_t index = (oldest_number + offset) % capacity;
    value = buffer[index];

    return true;
}
