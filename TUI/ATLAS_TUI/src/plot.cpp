#include "plot.h"
#include <iostream>

plot::plot()
{
    std::string kommando = "py plot.py";
    system(kommando.c_str());
}