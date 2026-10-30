#include "plot.h"
#include <iostream>

plot::plot()
{
    std::string kommando = "/home/au786842/SW3/SW3PRJ3_5/.venv/bin/python plot.py";
    system(kommando.c_str());
}