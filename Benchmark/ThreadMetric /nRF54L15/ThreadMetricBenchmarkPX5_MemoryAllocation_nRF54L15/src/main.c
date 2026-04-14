#include "tm_api.h"
void tm_main(void);
void tm_report_init(void);

#include "../../px5/source/pthread.h"

int main(void)
{
    px5_pthread_start(1, NULL, 0);
    tm_report_init();
    tm_main();
    for (;;) {}
}
