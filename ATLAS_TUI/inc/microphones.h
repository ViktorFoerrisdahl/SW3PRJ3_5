#ifndef MICROPHONES_H
#define MICROPHONES_H

#include <vector>

struct Vec2 
{
    int x, y;
};

class microphones
{
private:
    int AmountOfMicrophones_ = 0;
    std::vector<Vec2> Microphones_;
    
public:
    microphones(int amount);
    void AppendMicToVector(Vec2 micLocation);

    int get_AmountOfMicrophones() const;
    const std::vector<Vec2>& get_MicrophonesVector() const;
};


#endif