#ifndef __ASYNC_H__
#define __ASYNC_H__

// Rejection reason with_deadline() throws with when its ceiling elapses
// before the work settles. No leading '*' — that prefix marks a reason the
// driver authored, and this one is ours. Compare against this constant
// rather than the text; the wording is not the API.
#define ASYNC_ERR_TIMEOUT "async work did not settle in time"

// Rejection reason an iterating sefun throws with when the object owning its
// callback is destructed part way through. Also ours, so also no leading '*'.
#define ASYNC_ERR_OWNER_GONE "callback owner was destructed mid-iteration"

#endif // __ASYNC_H__
