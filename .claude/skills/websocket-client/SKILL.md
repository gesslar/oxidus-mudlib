---
name: websocket-client
description: Create WebSocket client daemons for Oxidus. Covers inheriting STD_WS_CLIENT, connecting, the handshake lifecycle, sending/receiving text and binary frames, ping/pong, graceful close, TLS/WSS, and subprotocols.
---

# WebSocket Client Skill

You are helping create or modify WebSocket client daemons for Oxidus. These are LPC daemons that maintain persistent WebSocket connections to external services. Follow the `lpc-coding-style` skill for all formatting.

## Architecture Overview

```
STD_DAEMON + M_HTTP
  └── STD_WS_CLIENT (std/daemon/websocket_client.c) — base WebSocket client
        └── Your daemon (e.g., adm/daemons/my_ws.c)
```

- **`STD_WS_CLIENT`** (`#define STD_WS_CLIENT DIR_STD "daemon/websocket_client"`) — base class implementing the full RFC 6455 WebSocket protocol: HTTP upgrade handshake, frame parsing/formatting, masking, ping/pong, and graceful close.
- **`M_HTTP`** (`std/modules/http.c`) — shared module for URL parsing, header parsing, etc. Inherited automatically.

## Required Include

```c
#include <websocket.h>
```

This provides WebSocket state constants, opcodes, and close codes.

## Creating a WebSocket Client Daemon

Inherit `STD_WS_CLIENT` and implement callback functions:

```c
#include <websocket.h>

inherit STD_WS_CLIENT;

void setup() {
  set_log_prefix("(MY WS)");
  set_log_level(0);

  call_out("start_connection", 3);
}

void start_connection() {
  mapping result = websocket_connect("wss://example.com/ws");

  if(result["status"] == "error")
    _log(0, "Connection failed: %s", result["error"]);
}

// Called when WebSocket handshake completes successfully
void websocket_handle_connected() {
  _log(1, "Connected!");

  // Send a text message
  websocket_message(WS_TEXT_FRAME, json_encode(([
    "type" : "hello",
    "data" : "world",
  ])));
}

// Called when a text frame is received
void websocket_handle_text_frame(mixed payload) {
  // payload is auto-parsed: JSON becomes a mapping, otherwise string
  if(mapp(payload)) {
    string type = payload["type"];
    // Handle message...
  }
}

// Called when connection shuts down
void websocket_handle_shutdown() {
  _log(1, "Disconnected");
  // Optionally reconnect
  call_out("start_connection", 10);
}
```

## Core Functions

### Connecting

```c
protected nomask mapping websocket_connect(string url)
```

Initiates a WebSocket connection. The URL scheme determines TLS:
- `ws://` — plain WebSocket
- `wss://` — WebSocket over TLS

Returns a mapping:
- `([ "status" : "success", "fd" : <fd> ])` — connection initiated
- `([ "status" : "error", "error" : "<message>" ])` — immediate failure (e.g., already connected, invalid URL, socket creation failed)

The connection process is asynchronous: DNS resolution, TCP connect, then HTTP upgrade handshake. Results arrive via callbacks.

**Important:** Only one connection per daemon instance. If `server` is already set, `websocket_connect()` returns an error.

### Sending Messages

```c
protected nomask varargs int websocket_message(int frame_opcode, mixed args...)
```

Sends a WebSocket frame. Returns `1` on success, `0` on failure.

| Opcode | Constant | Usage |
|---|---|---|
| Text | `WS_TEXT_FRAME` | `websocket_message(WS_TEXT_FRAME, "hello")` or `websocket_message(WS_TEXT_FRAME, json_encode(data))` |
| Binary | `WS_BINARY_FRAME` | `websocket_message(WS_BINARY_FRAME, buffer_data)` |
| Ping | `WS_PING_FRAME` | `websocket_message(WS_PING_FRAME, "")` or use `send_ping()` |
| Pong | `WS_PONG_FRAME` | `websocket_message(WS_PONG_FRAME, payload)` or use `send_pong(payload)` |
| Close | `WS_CLOSE_FRAME` | `websocket_message(WS_CLOSE_FRAME, code, reason)` or use `websocket_close()` |

All outgoing frames are masked (as required by RFC 6455 for clients).

### Convenience Functions

```c
protected nomask void send_ping()              // Send a ping frame
protected nomask void send_pong(string payload) // Send a pong frame (auto-called on ping receipt)
```

### Closing

```c
varargs protected nomask int websocket_close(int code, string reason)
```

Sends a close frame and shuts down. Both parameters are optional:
- `code` defaults to `WS_CLOSE_NORMAL` (1000)
- `reason` defaults based on the code (e.g., `"Disconnecting"` for 1000)

```c
websocket_close();                                    // Normal close
websocket_close(WS_CLOSE_GOING_AWAY, "Shutting down"); // Going away
```

### Shutdown

```c
protected nomask void shutdown_websocket()
```

Forcefully shuts down the connection without sending a close frame. Logs statistics and calls `websocket_handle_shutdown()`.

## Callback Lifecycle

Implement any of these in your daemon:

| Callback | When Called |
|---|---|
| `websocket_handle_resolved()` | DNS resolution succeeded |
| `websocket_handle_resolve_error()` | DNS resolution failed |
| `websocket_handle_connection_error(int result)` | TCP connection failed |
| `websocket_handle_handshake_sent()` | HTTP upgrade request sent |
| `websocket_handle_handshake_failed(string reason)` | Handshake validation failed |
| `websocket_handle_connected()` | **WebSocket established — ready to send/receive** |
| `websocket_handle_text_frame(mixed payload)` | Text frame received (payload is auto-parsed) |
| `websocket_handle_binary_frame(mapping frame_info)` | Binary frame received |
| `websocket_handle_close_frame(buffer payload)` | Close frame received from server |
| `websocket_handle_ping_frame(mapping frame_info)` | Ping received (pong is auto-sent) |
| `websocket_handle_pong_frame(mapping frame_info)` | Pong received |
| `websocket_handle_continuation_frame(mapping frame_info)` | Continuation frame received |
| `websocket_handle_message_sent(int opcode, mixed *args)` | Message successfully sent |
| `websocket_handle_message_error(int result)` | Message send failed |
| `websocket_handle_closed()` | Socket closed by remote |
| `websocket_handle_shutdown()` | Connection fully shut down |

### Text Frame Payload

For `websocket_handle_text_frame(mixed payload)`:
- If the text is valid JSON, `payload` is automatically decoded to an LPC mapping or array
- Otherwise, `payload` is the raw string

### Frame Info Mapping

For binary, ping, pong, and continuation callbacks, `frame_info` contains:

```c
([
  "fin"            : 128,           // FIN bit (0x80 if final frame)
  "opcode"         : WS_*_FRAME,    // Frame opcode
  "masked"         : 0,             // Whether server masked the frame
  "payload_length" : 1234,          // Payload size in bytes
  "payload"        : <buffer>,      // Raw payload buffer (or parsed for text)
  "buffer"         : <buffer>,      // Remaining unprocessed data
])
```

## The `server` Variable

The `server` mapping (a `protected nosave` variable) tracks connection state:

```c
([
  "fd"              : 5,                    // Socket file descriptor
  "start_time"      : 1234567890.123,       // Connection start time
  "websocket_state" : WS_STATE_CONNECTED,   // Current state
  "request"         : ([                    // Connection target
    "protocol"      : "wss",
    "secure"        : 1,
    "host"          : "example.com",
    "port"          : 443,
    "path"          : "/ws",
    "subprotocols"  : ({ }),
  ]),
  "response"        : ([                    // HTTP upgrade response
    "status"        : ([ "code" : 101, "message" : "Switching Protocols" ]),
    "headers"       : ([ ... ]),
  ]),
  "transactions"    : 42,                   // Total read transactions
  "received_total"  : 56789,               // Total bytes received
  "sent_total"      : 12345,               // Total bytes sent
])
```

Check `server` for `null` to determine if connected: `if(!server) return;`

## WebSocket States

Defined in `<websocket.h>`:

| Constant | Value | Meaning |
|---|---|---|
| `WS_STATE_RESOLVING` | 0 | DNS resolution in progress |
| `WS_STATE_CONNECTING` | 1 | TCP connection in progress |
| `WS_STATE_HANDSHAKE` | 2 | HTTP upgrade handshake in progress |
| `WS_STATE_CONNECTED` | 3 | WebSocket connection established |
| `WS_STATE_HANDSHAKE_FAILED` | 4 | Handshake validation failed |
| `WS_STATE_CLOSED` | 5 | Connection closed |
| `WS_STATE_ERROR` | 100 | Error occurred |

## WebSocket Opcodes

| Constant | Value | Description |
|---|---|---|
| `WS_TEXT_FRAME` | 0x01 | UTF-8 text data |
| `WS_BINARY_FRAME` | 0x02 | Binary data |
| `WS_CLOSE_FRAME` | 0x08 | Connection close |
| `WS_PING_FRAME` | 0x09 | Ping (keepalive) |
| `WS_PONG_FRAME` | 0x0A | Pong (keepalive response) |
| `WS_CONTINUATION_FRAME` | 0x00 | Continuation of fragmented message |

## WebSocket Close Codes

| Constant | Value | Meaning |
|---|---|---|
| `WS_CLOSE_NORMAL` | 1000 | Normal closure |
| `WS_CLOSE_GOING_AWAY` | 1001 | Server/client shutting down |
| `WS_CLOSE_PROTOCOL_ERROR` | 1002 | Protocol error |
| `WS_CLOSE_UNSUPPORTED_DATA` | 1003 | Unsupported data type |
| `WS_CLOSE_NO_STATUS_RECEIVED` | 1005 | No status code present |
| `WS_CLOSE_ABNORMAL` | 1006 | Abnormal closure (no close frame) |
| `WS_CLOSE_INVALID_FRAME_PAYLOAD_DATA` | 1007 | Invalid payload data |
| `WS_CLOSE_POLICY_VIOLATION` | 1008 | Policy violation |
| `WS_CLOSE_MESSAGE_TOO_BIG` | 1009 | Message too large |
| `WS_CLOSE_MANDATORY_EXTENSION` | 1010 | Required extension missing |
| `WS_CLOSE_INTERNAL_SERVER_ERROR` | 1011 | Server internal error |
| `WS_CLOSE_TLS_HANDSHAKE` | 1015 | TLS handshake failure |

## TLS/WSS

WSS URLs (`wss://`) are automatically handled:
- Creates a `STREAM_TLS_BINARY` socket
- Sets `SO_TLS_VERIFY_PEER` for certificate verification
- Sets `SO_TLS_SNI_HOSTNAME` for Server Name Indication

## Heartbeat Pattern

Many WebSocket services require periodic heartbeats. Use `call_out` to send pings:

```c
void websocket_handle_connected() {
  start_heartbeat();
}

void start_heartbeat() {
  remove_call_out("do_heartbeat");
  call_out("do_heartbeat", 30);
}

void do_heartbeat() {
  if(!server) return;
  send_ping();
  call_out("do_heartbeat", 30);
}
```

## Reconnection Pattern

```c
private nosave int reconnect_delay = 5;

void websocket_handle_shutdown() {
  _log(1, "Disconnected, reconnecting in %d seconds", reconnect_delay);
  call_out("start_connection", reconnect_delay);
  reconnect_delay = min(({ reconnect_delay * 2, 300 }));  // Exponential backoff, max 5 min
}

void websocket_handle_connected() {
  reconnect_delay = 5;  // Reset on successful connect
}
```

## Graceful Shutdown

The `shutdown()` apply sends a normal close frame. The `mudlib_unsetup()` apply sends a going-away close:

```c
void shutdown() {
  websocket_close();  // WS_CLOSE_NORMAL
}

void mudlib_unsetup() {
  shutdown(WS_CLOSE_GOING_AWAY);
}
```

Both are implemented in the base class automatically.

## Real-World Examples

### Grapevine (`adm/daemons/grapevine.c`)

MUD interconnection network client. Authenticates, sends heartbeats, broadcasts player login/logout events, handles incoming chat messages.

### Discord Bot (`lib/daemon/discord_bot.c`)

Discord gateway WebSocket client. Handles identify, heartbeat loop, and channel-based message handling.

### WebSocket Echo Test (`adm/daemons/websocket_echo.c`)

Simple test client that connects to an echo server, sends periodic messages, and verifies responses.
