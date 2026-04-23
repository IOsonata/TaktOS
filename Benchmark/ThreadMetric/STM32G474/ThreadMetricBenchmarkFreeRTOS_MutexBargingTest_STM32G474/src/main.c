#include <stdint.h>

extern void tm_main(void);
extern void tm_report_init(void);
extern void tm_hw_console_init(void);

int main(void)
{
    tm_hw_console_init();
    tm_report_init();
    tm_main();
    for (;;) {}
    return 0;
}
