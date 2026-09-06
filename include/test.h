#ifndef TESTS_H
#define TESTS_H

#include <driver/type.h>

#define SAFE(x) do {x} while(0)

#define CLEAR_ERROR ((master()->clear_last_error() || 1))

#define OUTPUT(x) SAFE(write(catch(error(x)));)
#define WHERE __FILE__ + ":" + __LINE__

#define ASSERT(x) if (CLEAR_ERROR && !(x)) { OUTPUT(WHERE + ", Check failed.\n"); }
#define ASSERT2(x, r) if (CLEAR_ERROR && !(x)) { OUTPUT(WHERE + ", Check failed: " + r + ".\n"); }
#define ASSERT_EQ(x, y) do { CLEAR_ERROR; _assert_eq((x), (y), WHERE); } while (0)
#define ASSERT_NE(x, y) do { CLEAR_ERROR; _assert_ne((x), (y), WHERE); } while (0)

// Seconds an atest() promise is given to settle before the runner gives up on
// it. A test that never settles would otherwise park the sweep forever, so
// this is a hard ceiling rather than a tuning knob; override async_timeout()
// in a test file that genuinely needs longer.
#define TEST_ASYNC_TIMEOUT 10

// Rejection reason the timeout arm throws with. No leading '*' — that prefix
// marks a reason the driver authored, and this one is ours.
#define TEST_ERR_TIMEOUT "test promise did not settle in time"

#endif
