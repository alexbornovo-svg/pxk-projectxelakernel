#include "timer.h"
#include "../lib/xelagraph.h"
#include "../lib/pic.h"
#include "io.h"

static volatile uint32_t timer_ticks = 0;

void timer_handler_c(void)
{
    timer_ticks++;
    if (timer_ticks % 18 == 0)
        topbar_refresh();
    outb(0x20, 0x20);
}

uint32_t timer_get_ticks(void) 
{ 
    return timer_ticks; 
}