#include <iostream>

#include "microphones.h"
#include "plot.h"
#include "menu.h"

microphones::microphones(int amount) : AmountOfMicrophones_(amount)
{
}

void microphones::replace(std::vector<MIC_POSITION> newMics)
{
    std::lock_guard<std::mutex> lock(dataLock_);
    Microphones_ = std::move(newMics);
}

std::vector<MIC_POSITION> microphones::snapshot() const
{
    std::lock_guard<std::mutex> lock(dataLock_);
    return Microphones_;
}

void microphones::inputMics()
{
      std::cout << "How many microphones? \n";
        int AmountOfMic;
        std::cin >> AmountOfMic;
        while (AmountOfMic < 0)
        {
            std::cout << "Amount of mics can't be negative number! Try again \n";
            std::cin >> AmountOfMic;
        }

        AmountOfMic = (static_cast<int>(AmountOfMic));

        std::vector<MIC_POSITION> tempVector;

        for (int i = 0; i < AmountOfMic; i++)
        {
            MIC_POSITION tempMicPos;
            std::cout << "What is the x position of microphone " << i << "?: \n";
            int temp_x_y{};
            std::cin >> temp_x_y;
            tempMicPos.x_m = temp_x_y;
            std::cout << "What is the y position of microphone " << i << "?: \n";
            std::cin >> temp_x_y;
            tempMicPos.y_m = temp_x_y;
            if (i < 2)
            {
                tempMicPos.channel = 0;
                tempMicPos.node_id = 0;
                if (i == 1)
                {
                    tempMicPos.channel = 1;
                    tempMicPos.node_id = 0;
                }
            }
            else
            {
                tempMicPos.channel = 0;
                tempMicPos.node_id = 1;
                if (i == 3)
                {
                    tempMicPos.channel = 1;
                    tempMicPos.node_id = 1;
                }
            }
            tempVector.push_back(tempMicPos);
        }

        this->replace(tempVector);
}