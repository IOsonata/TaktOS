/*
 * Thread-Metric main entry for Zephyr builds.
 * Zephyr starts the kernel before main() is called; tm_main() runs the test.
 * tm_main() lives in the per-project test file (basic_processing.c, etc.).
 */

extern void tm_main(void);

int main(void)
{
    tm_main();
    return 0;
}
