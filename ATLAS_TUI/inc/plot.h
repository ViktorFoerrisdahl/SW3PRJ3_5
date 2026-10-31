#ifndef PLOT_H
#define PLOT_H

#include <vector>
#include "microphones.h"

class plot
{
private:
    int EventID_;

public:
    plot();
    void startPyPlot(int AmountOfMics, std::vector<MIC_COORDINATES> vectorMic);
    void plotNewEvent(int ID);
};


#endif