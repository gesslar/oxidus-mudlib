/**
 * @file /std/daemon/http_server.c
 * @description This is an HTTP server that may be inherited by other objects
 *              to create a custom HTTP server. It listens on a specified port
 *              and accepts incoming connections.
 *
 * @created 2024-07-05 - Gesslar
 * @last_modified 2024-07-05 - Gesslar
 *
 * @history
 * 2024-07-05 - Gesslar - Created
 */

// /adm/daemons/http_server.c
// This is an HTTP server that may be inherited by other objects to create a
// custom HTTP server. It listens on a specified port and accepts incoming
// connections.
//
// Created:     2024/07/01: Gesslar
// Last Change: 2024/07/01: Gesslar
//
// 2024/07/01: Gesslar - Created

#include <socket.h>
#include <socket_err.h>
#include <http.h>
#include <daemons.h>

inherit STD_DAEMON;
inherit M_HTTP;

// Prototypes
protected nomask void send_http_response(int fd, mapping client);
protected nomask void close_client_connection(int fd);
protected nomask void close_all_client_connections();
protected nomask void socket_shutdown(int fd);

// Variables
private nomask nosave int LISTEN_PORT = 8080;
private nomask nosave mapping clients = ([ ]);
private nomask nosave mapping standard_headers = ([]);

void mudlib_setup() {
  standard_headers = ([
    "Server": "Oxidus (FluffOS)",
    "Connection": "close",
    "Date" : (: http_time_string :),
    "Content-Length": (: http_content_length :),
  ]);

  set_log_prefix("(HTTP SERVER)");
}

protected nomask int set_listen_port(int port) {
  if(port < 1 || port > 65535)
    return 0;

  LISTEN_PORT = port;

  return LISTEN_PORT;
}

protected nomask int query_listen_port() {
  return LISTEN_PORT;
}

protected nomask void start_server() {
  int fd;

  fd = socket_create(get_option("tls")
    ? STREAM_TLS_BINARY
    : STREAM_BINARY, "socket_read", "socket_close"
  );

  if(fd < 0) {
    _log(0, "Unable to create socket: %s", socket_error(fd));
    return;
  }

  if(get_option("tls")) {
    socket_set_option(fd, SO_TLS_CERT, "adm/certs/cert.pem");
    socket_set_option(fd, SO_TLS_KEY, "adm/certs/key.pem");
  }

  int bindStatus = socket_bind(fd, LISTEN_PORT);
  if(bindStatus != EESUCCESS) {
    _log(0, "Unable to bind to port %d: %s", LISTEN_PORT, socket_error(bindStatus));
    return;
  }

  int listenStatus = socket_listen(fd, "on_socket_listen");
  if(listenStatus != EESUCCESS) {
    _log(0, "Unable to listen on socket: %s", socket_error(listenStatus));
    return;
  }

  _log(1, "Listening on port %d", LISTEN_PORT);
}

protected nomask void on_socket_listen(int fd) {
  int client_fd;
  mapping client;
  string client_address;
  string client_host;
  int client_port;

  client_fd = socket_accept(fd, "socket_read", "socket_ready");
  if(client_fd < 0) {
    _log(0, "Error accepting client: %s", socket_error(client_fd));
    return;
  }
  _log(2, "Incoming connection on socket %d", client_fd);

  client_address = socket_address(client_fd);
  sscanf(client_address, "%s %d", client_host, client_port);

  client = ([
    "host": client_host,
    "port": client_port,
    "time": time_frac(),
    "http" : ([
        "request": ([ ]),
        "response": ([ ]),
    ]),
  ]);

  _log(2, "New connection from %s %d", client_host, client_port);

  clients[client_fd] = client;
}

void socket_ready(int fd) {
  _log(2, SYSTEM_OK "=> Socket ready: %d{{res}}", fd);
}

nomask void close_all_client_connections() {
  foreach(int fd, mapping _ in clients)
    close_client_connection(fd);
}

nomask void close_client_connection(int fd) {
  socket_shutdown(fd);
}

protected nomask void socket_shutdown(int fd) {
  mapping client = clients[fd];
  int result;

  _log(1, "Shutting down connection: %s %d", client["host"], client["port"]);

  if(!client)
    return;

  result = socket_close(fd);

  if(result != EESUCCESS)
    _log(0, "Error closing socket: %s", socket_error(result));
  else
    _log(3, "Removing socket: %s %d", client["host"], client["port"]);

  call_if(client, "http_handle_shutdown", client);

  map_delete(clients, fd);
}

protected nomask void socket_read(int fd, buffer incoming) {
  mixed err;
  mapping result;
  mapping client;
  string buf;

  _log(1, "Incoming data on %d: %O", fd, incoming);

  if(!bufferp(incoming))
    return;

  client = clients[fd];

  if(!client) {
    _log(0, "Unknown client: %d", fd);
    return;
  }

  buf = to_string(incoming);
  _log(3, "Received: %O", buf);

  err = catch(result = parse_http_request(incoming));

  _log(3, "Result: %O", result);

  if(err || !mapp(result)) {
    _log(2, "Error parsing: %O", err);

    client["response"] = ([
      "status": HTTP_STATUS_BAD_REQUEST,
      "content-type": CONTENT_TYPE_TEXT_PLAIN,
    ]);

    send_http_response(fd, client);

    return;
  }

  _log(3, "Parsed: %O", result);

  if(result == 0) {
    client["response"] = ([
        "status": HTTP_STATUS_BAD_REQUEST,
        "content-type": CONTENT_TYPE_TEXT_PLAIN,
    ]);

    send_http_response(fd, client);

    return;
  }

  clients[fd]["http"]["request"] = result;

  if(function_exists("http_handle_request")) {
    catch(call_other(this_object(), "http_handle_request", fd, client));
  } else {
    client["response"] = ([
      "status": HTTP_STATUS_NOT_IMPLEMENTED,
      "content-type": CONTENT_TYPE_TEXT_PLAIN,
    ]);

    send_http_response(fd, client);
  }
}

protected nomask void send_http_response(int fd, mapping client) {
  string response = "";
  mapping headers = standard_headers;
  mixed body;
  string status, content_type;
  int result;

  if(!fd || !client || !client["http"]["response"]["status"] || !client["http"]["response"]["content-type"])
    return;

  if(!client) {
    _log(0, "Unknown client: %d", fd);
    return;
  }

  body = client["http"]["response"]["body"];
  status = client["http"]["response"]["status"];

  if(body) {
    if(client["http"]["request"]["headers"]["accept-encoding"]) {
      string *accepts = client["http"]["request"]["headers"]["accept-encoding"];
      if(member_array("deflate", accepts) > -1 && get_option("deflate")) {
        headers["Content-Encoding"] = "deflate";
        body = compress(body);
        client["http"]["response"]["body"] = body;
      } else {
      }
    }
  }

  _log(3, "Sending body: %O", body);

  if(!body) {
    switch(status) {
      case HTTP_STATUS_OK:
      case HTTP_STATUS_CREATED:
      case HTTP_STATUS_ACCEPTED:
      case HTTP_STATUS_NO_CONTENT:
      case HTTP_STATUS_MOVED_PERMANENTLY:
      case HTTP_STATUS_FOUND:
      case HTTP_STATUS_NOT_MODIFIED:
      case HTTP_STATUS_BAD_REQUEST:
      case HTTP_STATUS_UNAUTHORIZED:
      case HTTP_STATUS_FORBIDDEN:
      case HTTP_STATUS_NOT_FOUND:
      case HTTP_STATUS_METHOD_NOT_ALLOWED:
      case HTTP_STATUS_INTERNAL_SERVER_ERROR:
      case HTTP_STATUS_NOT_IMPLEMENTED:
      case HTTP_STATUS_BAD_GATEWAY:
      case HTTP_STATUS_SERVICE_UNAVAILABLE:
        sscanf(status, "%*d %s", body);
        break;
      default: body = status; break;
    }
  }

  content_type = client["http"]["response"]["content-type"];

  // Build the HTTP response
  headers += ([
    "Content-Type": content_type,
  ]);

  foreach(string header, mixed value in headers) {
    mixed val;

    if(valid_function(value)) {
      function f = value;
      val = f(client);
    }
    else
        val = value;

    switch(typeof(val)) {
      case T_STRING:
        val = val;
        break;
      case T_INT:
        val = sprintf("%d", val);
        break;
      case T_FLOAT:
        val = sprintf("%f", val);
        break;
      default:
        throw(sprintf("Invalid type for header value: %O", val));
        break;
    }

    response += sprintf("%s: %s\r\n", header, val);
  }

  response += "\r\n"; // End of headers

  response = sprintf("HTTP/1.1 %s\r\n%s", status, response);

  // Send the response
  if(bufferp(body))  {
    result = socket_write(fd, response);
    if(result != EESUCCESS) {
      _log(0, "Error writing response: %s", socket_error(result));
      socket_shutdown(fd);
      return;
    }
    _log(2, "Headers sent");
    result = socket_write(fd, body);
    if(result != EESUCCESS) {
      _log(0, "Error writing response: %s", socket_error(result));
      socket_shutdown(fd);
      return;
    }
    _log(2, "Body sent");
  } else {
    response += body;
    result = socket_write(fd, response);
    if(result != EESUCCESS) {
      _log(0, "Error writing response: %s", socket_error(result));
      socket_shutdown(fd);
      return;
    }
    _log(2, "Response sent");
  }

  // Properly close the connection
  if(member_array("close", client["http"]["request"]["headers"]["connection"]) > -1) {
    _log(2, "Closing connection per client's request (%s %d)", client["host"], client["port"]);
    socket_shutdown(fd);
  } else if(get_option("close")) {
    _log(2, "Closing connection per server's request");
    socket_shutdown(fd);
  }
}
