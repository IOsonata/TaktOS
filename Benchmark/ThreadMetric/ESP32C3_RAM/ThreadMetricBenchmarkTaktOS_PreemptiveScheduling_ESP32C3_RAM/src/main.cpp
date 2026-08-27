extern "C" {
#include "tm_api.h"
void tm_main(void);
void tm_report_init(void);
void tm_hw_console_init(void);
}

int main(void)
{
    tm_hw_console_init();
    tm_report_init();
    tm_main();
    for (;;) {}
}
