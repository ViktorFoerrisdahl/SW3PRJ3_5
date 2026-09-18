#include <cstdlib>
#include <string>

void plotPosition(double x, double y)
{
    std::string kommando = "py position_plot.py " + std::to_string(x) + " " + std::to_string(y);
    system(kommando.c_str());
}

int main ()
{
    double x = 3.2;
    double y = 1.9;
    plotPosition(x, y);
    return 0;

}