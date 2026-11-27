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
    std::string command = "python3 plot.py " + std::to_string(AmountOfMics) + " ";
    for (auto m : vectorMic)
    {
        command.append(std::to_string(m.x_m));
        command.append(" ");
        command.append(std::to_string(m.y_m));
        command.append(" ");
    }

    {
        std::lock_guard<std::mutex> lock(python_lock_);

        FILE *plot = popen(command.c_str(), "w");
        if (plot == nullptr)
        {
            perror("popen in plot contructor");
            return;
        }
        plot_ = plot;
    }
}

void plot::plotNewEventAndDelete(int ID, double x, double y, int waittime, double rms)
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
    event.append(" ");
    event.append(std::to_string(rms));
    event.append("\n");

    {
        std::lock_guard<std::mutex> lock(python_lock_);
        fputs(event.c_str(), plot_);
        fflush(plot_);
    }

    sleep(waittime);

    std::string DEL;

    DEL.append("DEL");
    DEL.append("\n");

    {
        std::lock_guard<std::mutex> lock(python_lock_);
        fputs(DEL.c_str(), plot_);
        fflush(plot_);
    }
}

void plot::stopPyPlot()
{
    std::string STOP;

    STOP.append("STOP");
    STOP.append("\n");

    {
        std::lock_guard<std::mutex> lock(python_lock_);
        fputs(STOP.c_str(), plot_);
        fflush(plot_);
    }
}

void plot::EnterNewMics(microphones &mics)
{
    mics.inputMics();

    std::string NEWMICS;
    NEWMICS.append("MICS");
    NEWMICS.append(" ");
    NEWMICS.append(std::to_string(mics.snapshot().size()));
    NEWMICS.append(" ");

    for (auto m : mics.snapshot())
    {
        NEWMICS.append(std::to_string(m.x_m));
        NEWMICS.append(" ");
        NEWMICS.append(std::to_string(m.y_m));
        NEWMICS.append(" ");
    }

    NEWMICS.append("\n");

    {
        std::lock_guard<std::mutex> lock(python_lock_);
        fputs(NEWMICS.c_str(), plot_);
        fflush(plot_);
    }
}
