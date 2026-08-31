#ifndef __PROMISE_H__
#define __PROMISE_H__

// Static rejection reasons produced by the driver itself. Every one is a
// constant string, so a rejection reason can be compared against these to
// tell a driver-generated outcome from a reason your own code rejected with.
//
// The leading "*" is the driver's marker for a reason it authored; reasons
// you reject with should not carry it. None of these reach the error handler
// or the log — they are delivered outcomes, not faults, and arrive at an
// await() or acatch() exactly like any other rejection.

// Cancellation, delivered to an async body's next await by promise_cancel().
// Identical on every delivery path (body running, queued, or parked), so
// this one comparison catches all three.
#define PROMISE_REASON_CANCELLED         "*async function cancelled"

// The suspended body's owner went away underneath it. The body cannot
// continue in any of these cases; its promise rejects instead.
#define PROMISE_REASON_DESTRUCTED        "*async function owner was destructed while suspended"
#define PROMISE_REASON_RECOMPILED        "*async function owner was recompiled while suspended"
#define PROMISE_REASON_REPLACED_PROGRAM  "*async function owner's program was replaced while suspended"

// No stack left to rebuild the frame on when the body was resumed.
#define PROMISE_REASON_STACK_OVERFLOW    "*stack overflow while resuming async function"

// A promise died with its last reference before it settled, so whatever was
// waiting on it can never be told. Which one you see says who was waiting:
// ADOPTION_COLLECTED for a resolve-with-promise source, AWAITED_COLLECTED
// for a parked body, COLLECTED for a combinator input.
#define PROMISE_REASON_ADOPTION_COLLECTED "*promise adoption source was collected before settling"
#define PROMISE_REASON_AWAITED_COLLECTED  "*awaited promise was collected before settling"
#define PROMISE_REASON_COLLECTED          "*promise was collected before settling"

// promise_resolve(p, p) — a promise cannot adopt itself.
#define PROMISE_REASON_SELF_RESOLVED     "*promise resolved with itself"

// promise_any() over an empty array: nothing can ever fulfil it.
#define PROMISE_REASON_ANY_EMPTY         "*promise_any: no promises to wait for"

// await async_yield() that was still queued when the driver shut down.
#define PROMISE_REASON_YIELD_SHUTDOWN    "*async_yield never ran: the driver shut down"

// promise_reject(p) with no reason argument. The driver substitutes this so
// a bare reject is never falsy — `acatch` signals failure by yielding the
// reason, so a falsy reason would read as success.
#define PROMISE_REASON_REJECTED          "*promise rejected"

#endif // __PROMISE_H__
