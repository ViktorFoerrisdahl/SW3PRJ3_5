#ifndef MICROPHONES_H
#define MICROPHONES_H

#include <vector>

struct MIC_COORDINATES 
{
    int x, y;
};

class microphones
{
private:
    int AmountOfMicrophones_ = 0;
    std::vector<MIC_COORDINATES> Microphones_;
    
public:
    microphones(int amount);
    void AppendMicToVector(MIC_COORDINATES micLocation);

    int get_AmountOfMicrophones() const;
    const std::vector<MIC_COORDINATES>& get_MicrophonesVector() const;
};


#endif