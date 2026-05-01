#ifndef KVB_ASSERT_H
#define KVB_ASSERT_H

#include "kvb_result.h"

#define KVB_ASSERT_TRUE(expr, msg)       \
    do {                                 \
        if (!(expr)) {                   \
            result->state = KVB_RESULT_FAIL; \
            result->message = (msg);     \
            return;                      \
        }                                \
    } while (0)

#define KVB_ASSERT_STATUS_OK(expr, msg)  \
    do {                                 \
        if ((expr) != KVB_OK) {          \
            result->state = KVB_RESULT_FAIL; \
            result->message = (msg);     \
            return;                      \
        }                                \
    } while (0)

#endif /* KVB_ASSERT_H */
