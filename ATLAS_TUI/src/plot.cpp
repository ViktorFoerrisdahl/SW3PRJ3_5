#include "plot.h"
#include "microphones.h"
#include <iostream>
#include <unistd.h>
#include <vector>

plot::plot() : EventID_(0) {}

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