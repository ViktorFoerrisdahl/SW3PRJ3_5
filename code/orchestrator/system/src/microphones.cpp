#include "microphones.h"

microphones::microphones(int amount) : AmountOfMicrophones_(amount) {}

int microphones::get_AmountOfMicrophones() const
{
    return AmountOfMicrophones_;
}

const std::vector<MIC_POSITION>& microphones::get_MicrophonesVector() const
{
    return Microphones_;
}

void microphones::setAmoutOfMics(int amount)
{
    AmountOfMicrophones_ = amount;
}

void microphones::AppendMicToVector(MIC_POSITION micLocation)
{
    Microphones_.push_back(micLocation);
}

void microphones::clearMicVector()
{
    Microphones_.clear();
}
