#include <iostream>
#include <thread>
#include <unistd.h>

#include "menu.h"
#include "microphones.h"
#include "plot.h"

#define WAITTIME_BEFORE_DELETION 20

void init_TUI()
{
    menu menu;
    plot p;

    bool ATLAS_RUNNING = true;

    menu.showTitle();
    menu.showMenu();

    while (ATLAS_RUNNING)
    {
    
        microphones mics{0};
        
        mics.inputMics();

        menu.showStartingSequence();

        p.startPyPlot(mics.snapshot().size(), mics.snapshot());

        sleep(5);

        int input{23};

        while (ATLAS_RUNNING)
        {
            menu.showUserOptions();

            std::cin >> input;

            switch (input)
            {
            case 0:
            {
                p.stopPyPlot();
                std::cout << "ATLAS now closing! BYE! \n";
                ATLAS_RUNNING = false;
                break;
            }

            case 1:
            {
                mics.inputMics();
                p.EnterNewMics(mics);
                std::cout << "New MICS recieved and updated \n \n";
                break;
            }

            default:
                break;
            }
        }
    }

}