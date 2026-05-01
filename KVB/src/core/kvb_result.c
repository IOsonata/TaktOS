#include "kvb_config.h"
#include "kvb_result.h"

const char *kvb_result_state_name(KvbResultState state)
{
    switch (state) {
    case KVB_RESULT_PASS:
        return "PASS";
    case KVB_RESULT_FAIL:
        return "FAIL";
    case KVB_RESULT_UNSUPPORTED:
        return "UNSUPPORTED";
    case KVB_RESULT_SKIPPED:
        return "SKIPPED";
    case KVB_RESULT_INVALID:
        return "INVALID";
    default:
        return "INVALID";
    }
}
