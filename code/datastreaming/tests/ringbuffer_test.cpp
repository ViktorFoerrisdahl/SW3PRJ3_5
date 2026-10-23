#include "../include/ringbuffer.h"
#include <iostream>

int main()
{
    RingBuffer buffer(5);

    for (int value = 10; value <= 70; value += 10) {
        buffer.RingBuffer_Write(value);
    }

    for (std::size_t i = 0; i < buffer.RingBuffer_GetSize(); ++i) {
        int value;

        if (buffer.RingBuffer_Read(i, value)) {
            std::cout << value << ' ';
        }
    }

    std::cout << '\n';
}