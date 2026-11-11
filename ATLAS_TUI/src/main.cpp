#include <iostream>
#include <thread>
#include <unistd.h>

#include "menu.h"
#include "microphones.h"
#include "plot.h"

#define WAITTIME_BEFORE_DELETION 10

int main()
{
    menu menu;
    plot p;

    menu.showTitle();
    menu.showMenu();

    while (1)
    {
        std::cout << "How many microphones? \n";
        int tempMic;
        std::cin >> tempMic;
        while (tempMic < 0)
        {
            std::cout << "Amount of mics can't be negative number! Try again \n";
            std::cin >> tempMic;
        }
        microphones m(static_cast<int>(tempMic));

        for (int i = 0; i < m.get_AmountOfMicrophones(); i++)
        {
            MIC_POSITION tempVector;
            std::cout << "What is the x position of microphone " << i << "?: \n";
            int temp_x_y{};
            std::cin >> temp_x_y;
            tempVector.x_m = temp_x_y;
            std::cout << "What is the y position of microphone " << i << "?: \n";
            std::cin >> temp_x_y;
            tempVector.y_m = temp_x_y;
            if (i < 2)
            {
                tempVector.channel = 0;
                tempVector.node_id = 0;
                if (i == 1)
                {
                    tempVector.channel = 1;
                    tempVector.node_id = 0;
                }
            }
            else
            {
                tempVector.channel = 0;
                tempVector.node_id = 1;
                if (i == 3)
                {
                    tempVector.channel = 1;
                    tempVector.node_id = 1;
                }
            }
            m.AppendMicToVector(tempVector);
        }

        menu.showStartingSequence();

        p.startPyPlot(m.get_AmountOfMicrophones(), m.get_MicrophonesVector());

        sleep(5);

        std::thread t1([&p]() { p.plotNewEventAndDelete(1, 1, 2, WAITTIME_BEFORE_DELETION); });

        std::thread t2([&p]() { p.plotNewEventAndDelete(2, 2, 2, WAITTIME_BEFORE_DELETION); });

        menu.showUserOptions();

        int input{};
        std::cin >> input;

        switch (input)
        {
        case 0:
        {
            p.stopPyPlot();
            std::cout << "ATLAS now closing! BYE! \n";
            break;
        }

        case 1:
        {
        }

        default:
            break;
        }


        t1.join();
        t2.join();
        break;
       
    }

    return 0;
}