#include "plot.h"
#include "microphones.h"
#include <iostream>
#include <vector>

plot::plot() : EventID_(0) {}

void plot::startPyPlot(int AmountOfMics, std::vector<Vec2> vectorMic)
{


    std::string kommando = "/home/au786842/SW3/SW3PRJ3_5/.venv/bin/python plot.py " + std::to_string(AmountOfMics) + " ";
    for (auto m : vectorMic)
    {
        kommando.append(std::to_string(m.x));
        kommando.append(" ");
        kommando.append(std::to_string(m.y));
        kommando.append(" ");
    }
    system(kommando.c_str());
}