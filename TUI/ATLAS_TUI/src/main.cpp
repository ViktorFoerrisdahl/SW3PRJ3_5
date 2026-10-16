#include <iostream>

#include "menu.h"
#include "plot.h"

int main()
{
    menu m;

    m.showTitle();
    m.showMenu();

    while (1)
    {
        int input{};
        std::cin >> input;

        while (input == 1)
        {
            plot p;
        }
    }

    return 0;
}