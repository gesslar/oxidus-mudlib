---
title: GMCP
description: Out-of-band client/server communication in both directions.
---

GMCP (Generic MUD Communication Protocol) is an out-of-band channel between the MUD and the client. It carries structured data -- character vitals, room info, inventory, chat, client identification -- alongside the normal text stream. Oxidus implements GMCP in both directions: messages received **from** the client and messages sent **to** it.

## How It Works

The two directions use separate machinery:

- **Incoming** (client -> server) is handled by the `EXT_GMCP` module on the player and login objects, which parses each message and routes it to a handler file by package name.
- **Outgoing** (server -> client) flows through the GMCP daemon (`GMCP_D`), which routes a package to a daemon module that formats the payload and sends it.

All package names, keys, and values are defined in `include/gmcp_defines.h` and should be referenced by their `GMCP_*` macros rather than written as literal strings.

## Incoming: Client to Server

When a GMCP message arrives, the driver calls `gmcp(string message)` on the user or login object. That function lives in `std/ext/gmcp.lpc` (`EXT_GMCP`), inherited by both `std/living/player.lpc` and `adm/obj/login.lpc`. It does **not** gate on whether the player has GMCP enabled -- this ensures the `Core.Hello` and `Core.Supports` handshake still runs during login.

The message is parsed by `GMCP_D->convert_message()` into a `ClassGMCP`, which splits the dotted name into its parts:

```lpc
class ClassGMCP {
  string name;       // "Core.Supports.Set"
  string package;    // "Core"
  string module;     // "Supports"
  string submodule;  // "Set"
  mixed  payload;    // JSON-decoded data, or null
}
```

It is then routed to `std/handlers/gmcp/<Package>.lpc`, calling the function named after the module:

- `Core.Hello` -> `Core.lpc::Hello(payload)`
- `Core.Supports.Set` -> `Core.lpc::Supports("Set", payload)`
- `Char.Items.Inv` -> `Char.lpc::Items("Inv", null)`

If no handler file exists for the package, the miss is logged to `system/gmcp`.

### Supported Incoming Packages

| Package | Handler | Purpose |
|---|---|---|
| `Core.Hello` | `Core.lpc` | Stores client name/version (login only) |
| `Core.Supports.Set/Add/Remove` | `Core.lpc` | Tracks which packages the client supports |
| `Core.Ping` | `Core.lpc` | Echoes a pong (rate-limited, 60s cooldown) |
| `Char.Login.Credentials` | `Char.lpc` | Authenticates and replies with `Char.Login.Result` |
| `Char.Items.Inv/Room/Contents` | `Char.lpc` | Sends inventory, room, or container item lists |
| `External.Discord.Hello/Get` | `External.lpc` | Discord integration info |

Handler modules inherit `std/handlers/gmcp/gmcp_module.lpc`, which provides the per-player cooldown system used to rate-limit requests like `Core.Ping`.

## Outgoing: Server to Client

Game code sends data by calling the daemon:

```lpc
#include <daemons.h>
#include <gmcp_defines.h>

GMCP_D->send_gmcp(tp, GMCP_PKG_CHAR_VITALS);
```

`GMCP_D` identifies the package and dispatches to the matching daemon module under `adm/daemons/modules/gmcp/` (`Char`, `Client`, `Comm`, `Core`, `Room`). `do_gmcp()` is a no-op when the driver has GMCP disabled or the player has turned it off, so senders need not check first. That module assembles the payload -- when none is supplied it sends the full default set -- and calls `do_gmcp()` on the target to JSON-encode and transmit it. So a bare `send_gmcp(tp, GMCP_PKG_CHAR_VITALS)` causes the `Char` module to gather and send the player's current vitals.

## Waiting for a Message

Some flows need to *wait* for the client to answer rather than react whenever it
happens to. `gmcp_await()` -- also on `EXT_GMCP` -- opens a one-shot mailbox for
a full dotted package name and hands back a promise:

```lpc
// Give the client one second to declare what it supports.
promise supports = gmcp_await(GMCP_PKG_CORE_SUPPORTS_SET, 1.0);
mixed payload = await supports;
```

- The promise fulfils with the message payload, or rejects once the wait has
  gone unanswered for `seconds`. A non-positive `seconds` waits indefinitely,
  which only makes sense for something the client is certain to send or for
  something being raced against another arm.
- There is **one outstanding wait per package name**. A second call replaces the
  first, which is then settled by nothing but its own deadline.
- The promise handed back is always an async body's, so a caller racing it
  against something else can release the loser with `promise_cancel()`.

Waiters are woken before handler dispatch, and a message you are only waiting on
does not need a handler file to exist. The login flow uses both forms -- see
`adm/obj/login.lpc`. For the promise semantics themselves, see the
`async-promises` skill.

## Adding a Handler

**Incoming, existing package:** add a function named after the module to the handler file (e.g. `Skills()` in `Char.lpc`), and add any new `GMCP_PKG_*` define. Routing finds it automatically.

**Incoming, new package:** create `std/handlers/gmcp/<Package>.lpc`, `inherit __DIR__ "gmcp_module";`, and add the defines. The router locates it by file name.

**Outgoing:** add a function for the package to the appropriate daemon module under `adm/daemons/modules/gmcp/`, then call `GMCP_D->send_gmcp()` from game code.

## Key Files

| File | Role |
|---|---|
| `std/ext/gmcp.lpc` | `EXT_GMCP` -- incoming entry point, `gmcp_await()`, and the `do_gmcp()` sender |
| `std/handlers/gmcp/` | Incoming handler modules (`Core`, `Char`, `External`) |
| `std/handlers/gmcp/gmcp_module.lpc` | Handler base class -- cooldown system |
| `adm/daemons/gmcp.lpc` | `GMCP_D` -- parsing and outgoing routing |
| `adm/daemons/modules/gmcp/` | Outgoing daemon modules (`Char`, `Client`, `Comm`, `Core`, `Room`) |
| `std/classes/gmcp.lpc` | `ClassGMCP` structure |
| `include/gmcp_defines.h` | All `GMCP_*` package, key, and value defines |
