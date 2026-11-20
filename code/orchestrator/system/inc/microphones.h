#ifndef MICROPHONES_H
#define MICROPHONES_H

#include <vector>
#include <cstdint>
#include <mutex>
#include <utility>

struct MIC_POSITION 
{
    double x_m;
    double y_m;
    uint16_t node_id;
    uint16_t channel;
};

class microphones
{
private:
    int AmountOfMicrophones_ = 0;
    std::vector<MIC_POSITION> Microphones_;
    mutable std::mutex dataLock_;
    
public:
    microphones(int amount);
    
    void replace(std::vector<MIC_POSITION> newMics);
    std::vector<MIC_POSITION> snapshot() const;

    int get_AmountOfMicrophones() const;
    const std::vector<MIC_POSITION>& get_MicrophonesVector() const;

    void setAmoutOfMics(int amount);

};


#endif