extern "C" {
#include "tm_api.h"
void tm_main(void);
void tm_report_init(void);
}

int main(void)
{
    tm_report_init();
    tm_main();
    for (;;) {}
}
