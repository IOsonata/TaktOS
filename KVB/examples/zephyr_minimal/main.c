#include "kvb_zephyr_port.h"

int main(void)
{
    /* Zephyr starts the kernel before main(). KVB runs from this app thread. */
    return kvb_zephyr_start_default();
}
