#include "tm_api.h"

void tm_hw_console_init(void);
void tm_main(void);

int main(void)
{
    tm_hw_console_init();
    tm_report_init();
    tm_main();
    for (;;) {}
}
