#include <kclock.h>
#include <drivers/include/timer.h>

void kclock_init() {
    setup_clock_int(1000000);
}

