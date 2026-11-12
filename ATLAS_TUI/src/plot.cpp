#include "plot.h"
#include "microphones.h"
#include <iostream>
#include <unistd.h>
#include <vector>

plot::plot() : EventID_(0)
{
}

void plot::startPyPlot(int AmountOfMics, std::vector<MIC_POSITION> vectorMic)
{
    std::string command = "python plot.py " + std::to_string(AmountOfMics) + " ";
    for (auto m : vectorMic)
    {
        command.append(std::to_string(m.x_m));
        command.append(" ");
        command.append(std::to_string(m.y_m));
        command.append(" ");
    }

    FILE *plot = popen(command.c_str(), "w");
    if (plot == nullptr)
    {
        perror("popen in plot contructor");
        return;
    }
    plot_ = plot;
}

void plot::plotNewEventAndDelete(int ID, float x, float y, int waittime)
{
    if (plot_ == nullptr)
    {
        perror("plot_ == nulptr when sending");
        return;
    }

    sleep(2);

    std::string event;
    event.append("NEW");
    event.append(" ");
    event.append(std::to_string(x));
    event.append(" ");
    event.append(std::to_string(y));
    event.append(" ");
    event.append(std::to_string(ID));
    event.append("\n");

    fputs(event.c_str(), plot_);
    fflush(plot_);

    sleep(waittime);

    std::string DEL;

    DEL.append("DEL");
    DEL.append("\n");

    fputs(DEL.c_str(), plot_);
    fflush(plot_);
}

void plot::stopPyPlot()
{
    std::string STOP;

    STOP.append("STOP");
    STOP.append("\n");

    fputs(STOP.c_str(), plot_);
    fflush(plot_);
}

void plot::EnterNewMics(microphones &m)
{
    std::cout << "How many microphones? \n";
    int tempMic;
    std::cin >> tempMic;
    while (tempMic < 0)
    {
        std::cout << "Amount of mics can't be negative number! Try again \n";
        std::cin >> tempMic;
    }

    m.setAmoutOfMics((static_cast<int>(tempMic)));

    m.clearMicVector();

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

    std::string NEWMICS;
    NEWMICS.append("MICS");
    NEWMICS.append(" ");
    NEWMICS.append(std::to_string(m.get_AmountOfMicrophones()));
    NEWMICS.append(" ");

    for (auto m : m.get_MicrophonesVector())
    {
        NEWMICS.append(std::to_string(m.x_m));
        NEWMICS.append(" ");
        NEWMICS.append(std::to_string(m.y_m));
        NEWMICS.append(" ");
    }

    NEWMICS.append("\n");

    fputs(NEWMICS.c_str(), plot_);
    fflush(plot_);
}

