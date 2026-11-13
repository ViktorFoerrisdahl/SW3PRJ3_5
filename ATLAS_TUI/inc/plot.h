#ifndef PLOT_H
#define PLOT_H

#include <vector>
#include <stdio.h>
#include <mutex>
#include "microphones.h"

class plot
{
private:
    int EventID_;
    FILE* plot_ = nullptr;
    std::mutex python_lock_;
public:
    plot();
    void startPyPlot(int AmountOfMics, std::vector<MIC_POSITION> vectorMic);
    void plotNewEventAndDelete(int ID, float x, float y, int waittime);
    void stopPyPlot();
    void EnterNewMics(microphones &m);
};


#endif