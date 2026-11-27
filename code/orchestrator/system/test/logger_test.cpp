#include "json_logger.h"

int main()
{
    TDOA_RESULT t;
    t.valid = true;
    t.x_m = 12.4;
    t.y_m = 12.4;
    t.time_stamp_ns = 1795164733000000000; // 09:52:13

    JsonLogger_AppendEvent(t);
}