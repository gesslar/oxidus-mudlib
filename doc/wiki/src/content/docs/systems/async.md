---
title: Async
description: How async and await let slow things take their time without stopping the game.
---

Your MUD runs in a single lane. One command at a time, one room description at
a time, one combat round at a time. That is usually exactly what you want --
nothing happens halfway, and you never have to think about two bits of code
touching the same player at once.

The trouble starts when something has to *wait*. A spell with a two-second cast
time. A file you need to read off disk. A web request to some other service.
In a single lane, "waiting" means "everybody stops" -- and if your spell takes
two seconds to go off, every other player on the MUD stands frozen for those
two seconds.

`async` and `await` are how you get out of that. They let one piece of code
step out of the lane while it waits, and step back in when whatever it was
waiting for is ready. Everyone else keeps playing the whole time.

## When You Need It

You need `async` when your code has to wait for something real:

| You want to... | Reach for |
|---|---|
| Give an ability a wind-up before it lands | `await tp->async_act("punch", 2.0)` |
| Pause for a few seconds with nothing else attached | `await call_out_walltime(3.0)` |
| Read or write a file, or list a directory | `await async_read(path)` and friends |
| Ask a website or another service for something | the HTTP client -- it is async underneath |
| Do a big pile of work without locking up the game | a loop with `await async_yield()` |

And -- this is the part people get wrong -- you do **not** need `async` the
rest of the time. It is not a faster or more modern way to write a function. If
your code does not wait for anything, writing it the ordinary way is the right
answer, and it will be easier to read. Reach for `async` when there is a wait to
get out of, and not before.

## Your First Async Function

Two keywords do almost all the work. Both come from FluffOS itself rather than
from Oxidus -- the driver knows how to park a function part-way through and pick
it up again later, which is why this works as cleanly as it does.

`async` goes on a function to say "this one might need to wait". `await` goes in
front of the thing you are waiting *for*:

```lpc
private async void cast_fireball(object caster, object victim) {
  // Everything stops here for two seconds -- but only for this function.
  await caster->async_act("fireball", 2.0);

  caster->targetted_action("$N $vhurl a ball of fire at $t!", victim);
}
```

That is the whole idea. The function pauses at the `await`, the rest of the MUD
carries on, and when the two seconds are up the function picks up on the next
line exactly where it left off.

## The One Rule That Will Bite You

**Commands must not be `async`.**

When a player types something, the driver runs your command and reads what it
gives back *straight away* -- it will not wait around. An `async` command hands
back an unfinished answer, and the driver reads that as "sure, fine, that
worked", whether it did or not.

So the shape is always the same: keep the command itself ordinary, and have it
kick off an async helper without waiting for it.

```lpc
mixed use(object tp, string arg) {
  object victim;

  if(!victim = local_target(tp, arg, (: living($1) && $1 != $(tp) :)))
    return 1;

  tp->simple_action("$N $vpull back a fist...");

  strike(tp, victim);   // deliberately not awaited -- let it run on its own

  return 1;             // the command answers immediately
}

private async void strike(object tp, object victim) {
  if(!await tp->async_act("punch", 2.0))
    return;

  // ...land the punch...
}
```

`use()` answers the player right away. `strike()` goes off and does the slow
part on its own time. This is [`cmds/ability/punch.lpc`](https://github.com/gesslar/oxidus-mudlib/blob/main/cmds/ability/punch.lpc)
almost verbatim, and it is the pattern to copy.

The same goes for anything else the driver calls and reads immediately --
`create()`, `id()`, `heart_beat()`, and the rest. The compiler stops you putting
`async` on most of those by name, but it cannot catch every case, so know the
rule rather than relying on the warning.

:::caution
An `async` helper you never wait for is on its own. If it hits an error, nobody
is listening, and all you get is a line in the debug log a long way from the
cause. Wrap the body in `acatch` and log it yourself -- see
[When Things Go Wrong](#when-things-go-wrong).
:::

## Promises

When you call an async function, you do not get its answer -- it has not
finished yet. You get a **promise**: a receipt that says "an answer is coming".

```lpc
promise p = fetch_player_count();   // no answer yet, just the receipt
int count = await p;                // now wait for it and take the number
```

Most of the time you will not think about promises at all, because `await`
turns the receipt back into the answer and you write the two on one line:

```lpc
int count = await fetch_player_count();
```

A promise ends up in one of two places: it **fulfils** with a value, or it
**rejects** with a reason. It only ever settles once. And promises are ordinary
values -- you can put them in a variable, pass them around, or keep an array of
them, which is what makes the next section possible.

Promises also remember what kind of answer is coming. A function declared
`async int` gives you a promise carrying an `int`, so `int n = await f();`
works and `string s = await f();` is caught as a mistake when the file compiles
rather than going wrong in play.

## Waiting for Several Things at Once

Say you want to check ten rooms for something. Doing them one after another
means ten waits, back to back. Starting all ten and then waiting once for the
lot is much better, and there are helpers for exactly that:

| Helper | What it does |
|---|---|
| `promise_all` | Waits for all of them. If any one fails, the whole thing fails. |
| `promise_all_settled` | Waits for all of them and tells you how each one went. Never fails. |
| `promise_any` | Gives you the first one that succeeds. |
| `promise_race` | Gives you the first one to finish, succeed or fail. |

```lpc
mixed *results = await promise_all(map(names, (: fetch($1) :)));
```

Results come back in the order you asked for them, not the order they finished,
so `results[3]` always belongs to `names[3]`.

Which to pick usually comes down to one question: if three of your ten checks
fail, is that a *problem* or just a *result*? If it is a problem, `promise_all`
and let it fail. If some failing is normal -- which it usually is when you are
sweeping over lots of rooms or players -- use `promise_all_settled` and look at
what came back.

## Putting a Time Limit on Something

Waiting forever is a real risk when you are talking to anything outside the
MUD. `with_deadline()` puts a ceiling on it:

```lpc
mixed err = acatch {
  mixed answer = await with_deadline(ask_the_website(), 5.0);
};
```

Five seconds, then it gives up and you handle it. Use this rather than building
your own timer -- it cleans up after itself on every path, which is fiddly to
get right by hand.

One thing to be clear about: a deadline stops you *waiting*. It does not stop
the work. If you also need the thing itself to stop -- close a socket, cancel a
request -- do that yourself when the deadline fires.

## When Things Go Wrong

`acatch` is `catch` for code that waits. Ordinary `catch` cannot see past an
`await`; `acatch` can:

```lpc
mixed err = acatch {
  string text = await async_read("some/file");
};

if(err)
  _error(tp, "Could not read that file.");
```

Note the semicolon after the closing brace -- `acatch` is an expression, not a
block, and it is easy to forget.

There is one thing worth knowing here that catches people out. If an async
function hits an error, the error is reported from *inside that function*, at
the moment it goes wrong -- not where you caught it. Handling it neatly at your
end does not stop it being logged and does not stop the dev team being pinged.

So if you are writing an async function and a failure is something the caller is
*expected* to deal with -- a target that walked away, a file that is allowed to
be missing -- raise it with `throw()` inside your function rather than
`error()`. That is the difference between a quiet, handled outcome and paging
everybody at three in the morning. You have to decide that where the problem
happens; you cannot decide it at the catch.

## Stopping Something Early

`promise_cancel()` asks an async function to give up. It is a *request*, not a
kill switch -- the function finds out at its next `await` and can tidy up, or
even decline and carry on.

For anything the game world does to a character, though, reach for
`cancel_act()` instead. That interrupts the *act* -- the wind-up on the punch,
the cast time on the spell -- which is what you actually mean when someone gets
staggered or walks out of the room mid-cast. It resolves the wait cleanly and
the code that was waiting handles the interruption as a normal outcome, which is
why the punch example above checks what `async_act()` gave back:

```lpc
if(!await tp->async_act("punch", 2.0))
  return;    // something interrupted the wind-up; drop the punch
```

## Long Jobs

Sometimes the problem is not waiting, it is *volume* -- a job so big that
grinding through it in one go would lock the game up, or trip the driver's limit
on how long a single run may take.

`async_yield()` is the fix. It costs no time at all; it just hands the game a
turn:

```lpc
foreach(object room in every_room) {
  await async_yield();   // let everyone else have a go
  check_room(room);
}
```

The room crawler in `adm/daemons/crawler.lpc` works exactly this way. Note that
a plain `await` on its own will *not* do this -- it resumes too eagerly to let
anything else in. If your goal is to be polite about a long job, `async_yield()`
is the one you want.

## The Toolbox

Some of this comes from FluffOS itself and some of it Oxidus adds on top. Worth
knowing which is which. The driver ones all have in-game help -- `help
async_read`, `help promise_all` and so on -- because the driver's own
documentation is mirrored into `doc/driver/efun/` and folded into the developer
help.

### From the driver

| Function | What it is for |
|---|---|
| `async_read` / `async_write` / `async_getdir` | File work that does not hold up the game. Each takes a path first. Leave the trailing callback off and you get a promise back instead. |
| `async_db_exec` | The same idea for a database query, given a handle and the query. |
| `async_yield` | Hand the game a turn in the middle of a long job. |
| `promise_all` / `promise_any` / `promise_race` / `promise_all_settled` | Wait on a batch of things. Each takes an array -- see [above](#waiting-for-several-things-at-once). |
| `promise_cancel` | Ask an async function to give up. |
| `promise_create` / `promise_resolve` / `promise_reject` | Build and settle a promise by hand, when nothing else fits. |

### Added by Oxidus

| Function | What it is for |
|---|---|
| `async_act` | A wind-up on a character -- you give it an action name and a delay. Better than a plain timer, because it registers a proper act, so `cancel_act()` and anything else that interrupts still works. Gives you `1` if it finished, `0` if it was interrupted. |
| `with_deadline` | Wait for something, but not forever. Takes the thing to wait on and how long to give it. |
| `each_async` | Walk an array one item at a time, being polite about it. For work that is expensive to *do*. |
| `map_async` | Start every item at once and collect the lot. For work that is expensive to *wait for*. |
| `settle_async` | Same, but tells you how each one went instead of failing on the first problem. |
| `async_call_back` | Run a callback and wait for what it gives back. |
| `pendingp` / `settledp` / `resolvedp` / `rejectedp` / `cancelledp` | Ask a promise how it is doing. |

Anything you add to that second list needs its prototype -- `async` and all --
in `adm/include/simul_efun.h`. Nothing catches it at runtime if you forget.

## Common Mistakes

- **Making a command `async`.** See [above](#the-one-rule-that-will-bite-you).
  Keep the command ordinary; hand the slow part to a helper.
- **Forgetting the semicolon after `acatch { ... };`** -- it is an expression.
- **Using `async` on something that never waits.** It adds a promise and a layer
  of indirection and buys you nothing. Write it plainly.
- **Adding a prototype that disagrees with the definition.** If the function is
  `async`, the prototype says `async` too, and so does any override of it. The
  compiler will tell you.
- **`await a + b`** does not mean what it looks like. `await` grabs `a` first,
  then adds `b`. Put brackets round it if you meant the other thing.
- **Assuming a cancel stopped the work.** Cancelling stops your *waiting*.
  Whatever you were waiting on carries on unless you stop it too.

## Going Deeper

This page covers what you need to write ordinary game content. The `async-promises`
skill under `.claude/skills/` has the full picture -- building promises by hand,
running external processes, the scheduler, resource limits, and every way async
code can fail.

## Key Files

| File | Role |
|---|---|
| `std/living/act.lpc` | `async_act()` -- timed acts on a character |
| `adm/simul_efun/promise.lpc` | `with_deadline()` |
| `adm/simul_efun/array.lpc` | `each_async()`, `map_async()`, `settle_async()` |
| `adm/simul_efun/function.lpc` | `async_call_back()` |
| `adm/simul_efun/predicates.lpc` | `pendingp()`, `settledp()`, and the rest |
| `include/async.h` | `ASYNC_ERR_TIMEOUT` and friends |
| `doc/driver/efun/` | The driver's own docs for `async_*` and `promise_*`, mirrored in and searchable with `help` |
| `cmds/ability/punch.lpc` | A worked example of the command-plus-helper shape |
