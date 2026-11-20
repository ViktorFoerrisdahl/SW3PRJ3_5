#include "microphones.h"


microphones::microphones(int amount) : AmountOfMicrophones_(amount) {}



void microphones::replace(std::vector<MIC_POSITION> newMics){
    std::lock_guard<std::mutex> lock(dataLock_);
    Microphones_ = std::move(newMics);
}

std::vector<MIC_POSITION> microphones::snapshot() const{

    std::lock_guard<std::mutex> lock(dataLock_);
    return Microphones_;

}
