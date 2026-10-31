#include "microphones.h"

microphones::microphones(int amount) : AmountOfMicrophones_(amount) {}

int microphones::get_AmountOfMicrophones() const
{
    return AmountOfMicrophones_;
}

const std::vector<MIC_COORDINATES>& microphones::get_MicrophonesVector() const
{
    return Microphones_;
}

void microphones::AppendMicToVector(MIC_COORDINATES micLocation)
{
    Microphones_.push_back(micLocation);
}