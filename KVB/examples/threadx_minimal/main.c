#include "kvb_threadx_port.h"

int main(void)
{
    /* Hardware, clock, and console init belong here before entering ThreadX. */
    return kvb_threadx_start_default();
}
