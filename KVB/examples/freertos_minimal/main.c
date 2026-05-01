#include "kvb_freertos_port.h"

int main(void)
{
    /* Hardware, clock, and console init belong here before starting KVB. */
    return kvb_freertos_start_default();
}
