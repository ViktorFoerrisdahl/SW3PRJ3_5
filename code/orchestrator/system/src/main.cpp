#include <thread>

#include "TUI_thread.h"

int main()
{
    std::thread TuiThread(init_TUI);

    TuiThread.join();

    return 0;
}