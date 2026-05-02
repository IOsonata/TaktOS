/***************************************************************************
 * Temporary secure-stack stubs for Cortex-M33 non-secure build.
 *
 * These symbols are referenced by the ThreadX Cortex-M33 scheduler/port
 * when secure stack support hooks are enabled by the selected port config.
 *
 * This project does not currently provide TrustZone secure-stack runtime,
 * so provide minimal no-op implementations to satisfy linking.
 ***************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

void _tx_thread_secure_stack_context_save(void)
{
    /* No secure context to save in this build. */
}

void _tx_thread_secure_stack_context_restore(void)
{
    /* No secure context to restore in this build. */
}

void *_tx_thread_secure_mode_stack_allocate(void)
{
    /* Return a non-NULL sentinel so callers that check for allocation
       success continue to run in non-secure-only mode. */
    return (void *)1;
}

void _tx_thread_secure_mode_stack_free(void)
{
    /* Nothing to free in this build. */
}

void _tx_thread_secure_mode_stack_initialize(void)
{
    /* Nothing to initialize in this build. */
}

void _tx_thread_secure_stack_initialize(void)
{
    /* Global secure stack subsystem init not used in this build. */
}

void _tx_thread_secure_stack_free(void)
{
    /* Per-thread secure stack cleanup not used in this build. */
}

#ifdef __cplusplus
}
#endif
