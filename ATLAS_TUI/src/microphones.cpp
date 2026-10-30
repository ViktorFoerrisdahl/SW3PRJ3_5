#include "microphones.h"

microphones::microphones(int amount) : AmountOfMicrophones_(amount) {}

int microphones::get_AmountOfMicrophones() const
{
    return AmountOfMicrophones_;
}

const std::vector<Vec2>& microphones::get_MicrophonesVector() const
{
    return Microphones_;
}

void microphones::AppendMicToVector(Vec2 micLocation)
{
    Microphones_.push_back(micLocation);
}