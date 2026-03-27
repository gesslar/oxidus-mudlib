/**
 * @file /std/daemon/http_client.c
 * @description This is a base HTTP client daemon that is intended to be
 *              inherited by other objects to create a custom HTTP client. It
 *              provides the ability to send HTTP requests to a server and
 *              receive responses.
 *
 * @created 2024-07-05 - Gesslar
 * @last_modified 2024-12-07 - Gesslar
 *
 * @history
 * 2024-07-05 - Gesslar - Created
 * 2024-12-07 - Gesslar - Enhance HTTP client logging and response processing.
 *                        Improved cache file checks with detailed logging for
 *                        existence and size. Adjusted buffer handling in
 *                        chunked transfer encoding and content-length
 *                        scenarios. Cleaned up response processing logic for
 *                        better clarity and reliability.
 */

// This is a base HTTP client daemon that is intended to be inherited by other
// objects to create a custom HTTP client. It provides the ability to send HTTP
// requests to a server and receive responses.
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
varargs private nomask void http_connect(mapping request);
private nomask void process_redirect(int fd, mapping server);
private nomask void process_response(int fd, mapping server);
private nomask mixed send_http_request(int fd, mapping server);
protected nomask void shutdown_socket(int fd);
private nomask void socket_resolve(string host, string addr, int key);

// Variables
private nosave mapping servers = ([ ]);
private nosave mapping resolve_keys = ([ ]);
private nosave int max_redirects = 5;
private nosave mapping default_headers;

void mudlib_setup() {
  set_log_level(0);
  set_log_prefix("(HTTP CLIENT)");
  default_headers = ([
    "User-Agent" : "FluffOS",
    "Connection" : "close",
    "Accept-Charset" : "utf-8",
  ]);

  set_option("cache", "/tmp/http/");
  // set_option("deflate", 1);
}

void post_setup_1() {
  string cache = get_option("cache");

  assure_dir(cache);

  if(get_option("deflate"))
    default_headers["Accept-Encoding"] = "deflate";
  else
    default_headers["Accept-Encoding"] = "identify";
}

varargs nomask mapping http_request(string url, string method, mapping headers, string body) {
  mapping parsed_url;
  float now = time_frac();

  _log(2, "Url: %s, Method: %s, Headers: %O, Body: %s", url, method, headers, body);

  if(!stringp(url) || !stringp(method))
    error("Invalid URL, method, or headers specified.");

  parsed_url = parse_url(url);
  if(!parsed_url) {
    _log(2, "Failed to parse URL: %s", url);
    return null;
  }

  parsed_url["url"] = url;
  parsed_url["method"] = method;
  parsed_url["headers"] = headers;
  parsed_url["body"] = body;
  parsed_url["start_time"] = now;

  call_out_walltime((: http_connect, parsed_url :), 0.01);

  return copy(parsed_url);
}

varargs private nomask void http_connect(mapping request) {
  int fd, key;
  string host;
  mapping server;
  int secure;

  host = request["host"];

  secure = get_option("tls") || request["secure"];

  _log(2, "Connecting to %s", host);
  _log(4, "Request before creating socket: %O", request);

  fd = socket_create(secure ? STREAM_TLS_BINARY : STREAM_BINARY, "socket_read", "socket_closed");

  if(fd < 0) {
    _log(0, "Unable to create socket: %s", socket_error(fd));
    return;
  }

  if(secure) {
    socket_set_option(fd, SO_TLS_VERIFY_PEER, 1);
    socket_set_option(fd, SO_TLS_SNI_HOSTNAME, host);
  }

  server = ([
    "state" : HTTP_STATE_RESOLVING,
    "request" : request,
    "redirects" : 0,
  ]);

  _log(4, "Server after creating socket and options: %O", server);

  servers[fd] = server;

  key = resolve(host, (: socket_resolve :));
  resolve_keys[key] = fd;

  _log(2, "Resolving host: %s", host);
}

nomask void socket_resolve(string host, string addr, int key) {
  int fd, port, result;
  mapping server;

  _log(2, "Validating resolution for %s", host);

  if(nullp(resolve_keys[key]))
    return;

  fd = resolve_keys[key];
  map_delete(resolve_keys, key);

  server = servers[fd];
  _log(4, "Server: %O", server);

  if(!server)
    return;

  port = server["request"]["port"];

  if(!addr) {
    server["state"] = HTTP_STATE_ERROR;
    server["error"] = "Failed to resolve hostname " + host;
    servers[fd] = server;

    call_if(this_object(), "http_handle_resolve_error", server);

    _log(2, "Failed to resolve hostname %s", host);

    shutdown_socket(fd);
    return;
  }

  _log(2, "Host resolved: %s (%s)", host, addr);

  server["ip"] = addr;
  server["state"] = HTTP_STATE_CONNECTING;
  server["host"] = host;
  server["port"] = port;
  servers[fd] = server;

  call_if(this_object(), "http_handle_connecting", server);

  result = socket_connect(fd, addr + " " + port, "socket_read", "socket_ready");
  if(result != EESUCCESS) {
    _log(2, "Failed to connect to %s", host);

    server["state"] = HTTP_STATE_ERROR;
    server["error"] = socket_error(result);
    servers[fd] = server;

    call_if(this_object(), "http_handle_connection_error", server);

    shutdown_socket(fd);
    return;
  }

  server["state"] = HTTP_STATE_CONNECTED;
  server["transactions"] = 0;
  server["cache"] = get_option("cache") + time_ns();
  server["received_body"] = 0;
  server["received_total"] = 0;
  servers[fd] = server;
  _log(2, "Connected to %s %d", host, port);

  call_if(this_object(), "http_handle_connected", server);
}

nomask void socket_closed(int fd) {
  mapping server = servers[fd];

  if(!server) {
    _log(3, "No server found to be closed.");
    return;
  }

  _log(2, "Socket closed: %s %d", server["host"], server["port"]);

  call_if(this_object(), "http_handle_closed", server);

  shutdown_socket(fd);
}

nomask void shutdown_socket(int fd) {
  mapping server = servers[fd];
  int result;
  string cache_file;
  float duration;
  float speed;
  float now, started;
  float received_total;
  int fs;

  _log(3, "Shutting down socket: %d", fd);

  if(!server)
    return;

  cache_file = server["cache"];
  fs = file_size(cache_file);
  if(fs == -1) {
    _log(2, "Cache file does not exist: %s", cache_file);
  } else if(fs == 0) {
    _log(2, "Cache file is empty: %s", cache_file);
  } else {
    _log(2, "Total received: %d", server["received_total"]);
    _log(2, "Body received: %d", server["received_body"]);

    catch(process_response(fd, server));
  }

  now = time_frac();
  started = server["request"]["start_time"];
  duration = (now - started);

  received_total = to_float(server["received_total"]);
  speed = received_total / duration;

  _log(3, "Transfer size: %d bytes", server["received_total"]);
  _log(3, "Transfer time: %.2f seconds", duration);
  _log(3, "Transfer speed: %.2f bytes/s", speed);

  result = socket_close(fd);
  if(result != EESUCCESS)
    _log(2, "Error closing socket: %s", socket_error(result));
  else
    _log(3, "Socket removed: %s %d", server["host"], server["port"]);

  call_if(this_object(), "http_handle_shutdown", server);

  map_delete(servers, fd);
}

nomask void socket_read(int fd, buffer incoming) {
  mapping server = servers[fd];
  int status_code;
  buffer buf;

  if(!server)
    return;

  _log(4, "First 10 bytes: %O", incoming[0..9]);

  server["transactions"] ++;

  _log(3, "================= STARTING TRANSACTION #%d =================", server["transactions"]);

  if(server["buffer"]) {
    buf = server["buffer"] + incoming;
    map_delete(server, "buffer");
  } else
    buf = incoming;

  server["received_total"] += sizeof(buf);
  // Process headers if not done yet
  server["response"] = server["response"] || ([]);
  _log(4, "Server[response]: %O", server["response"]);

  _log(3, "Starting to process status");
  if(sizeof(buf) && !server["response"]["status"]) {
    mapping status;

    status = parse_response_status(buf, 1);
    if(!status) {
      server["buffer"] = buf;
      servers[fd] = server;
      return;
    }

    if(status["buffer"]) {
      buf = status["buffer"];
      map_delete(status, "buffer");
    }

    server["response"]["status"] = status;

    _log(3, "Status found: %O", status);
    _log(3, "Remaining buffer size: %d", sizeof(buf));
  } else
    _log(3, "Status already found");

  _log(3, "Starting to process headers");
  if(sizeof(buf) && !server["response"]["headers"]) {
    mapping headers;

    headers = parse_headers(to_string(buf), 1);

    if(!headers) {
      server["buffer"] = buf;
      servers[fd] = server;
      return;
    }

    if(headers["buffer"]) {
      buf = headers["buffer"];
      map_delete(headers, "buffer");
    }

    server["response"]["headers"] = headers;

    _log(4, "Headers found: %O", headers);
    _log(3, "Remaining buffer size: %d", sizeof(buf));
  } else
    _log(3, "Headers already found");

  if(!sizeof(server["response"]["status"]) || !sizeof(server["response"]["headers"])) {
    _log(3, "* Both status and headers not found");
    server["buffer"] = buf;
    servers[fd] = server;
    return;
  }

  if(sizeof(server["response"]["status"]) && sizeof(server["response"]["headers"]))
    _log(3, "Status and headers found");

  // Check for redirect status codes (301, 302, etc.)
  status_code = server["response"]["status"]["code"];

  if(member_array(status_code, HTTP_REDIRECT_CODES) != -1) {
    _log(1, "Redirecting");

    call_if(this_object(), "http_handle_redirect", server);

    server["buffer"] = buf;
    servers[fd] = server;
    process_redirect(fd, server);
    return;
  }

  server["received_body"] += sizeof(buf);

  _log(3, "Received body: %d, caching response", sizeof(buf));
  cache_response(server["cache"], buf);
}

protected nomask void socket_ready(int fd) {
  mapping server = servers[fd];
  mixed result;

  if(!server)
    return;

  server["state"] = HTTP_STATE_READY;
  servers[fd] = server;

  _log(2, "Ready to send request");

  call_if(this_object(), "http_handle_ready", server);

  server["state"] = HTTP_STATE_SENDING;
  servers[fd] = server;

  call_if(this_object(), "http_handle_sending", server);

  _log(2, "Sending request");
  _log(3, "Request: %O", server["request"]);
  result = send_http_request(fd, server);
  if(stringp(result)) {
    _log(2, "Failed to send request: %s", result);
    server["state"] = HTTP_STATE_ERROR;
    servers[fd] = server;
    shutdown_socket(fd);
    return;
  }
}

private nomask void process_response(int fd, mapping server) {
  string file = server["cache"];
  mixed response = "";
  mixed err;
  buffer cache;
  string buf;
  string rn = "\\r\\n", capture = "("+rn+")", replace = "";

  _log(2, "Processing response: %s", file);

  if(!server["response"]["body"])
    server["response"]["body"] = 0;

  cache = read_cache(file);
  buf = to_string(cache);
  _log(3, "Last 10 bytes of converted cache: %O", identify(buf[<10..]));

  // Chunked transfer encoding
  if(server["response"]["headers"]["transfer-encoding"] == "chunked") {
    mixed *assoc;
    string *parts;
    int *indices;
    int i, sz;
    int end_found;
    int total_size;

    _log(2, "Chunked transfer encoding found");

    assoc = pcre_assoc(buf, ({
      "(\\r\\n0\\r\\n\\r\\n)",      // End of chunked transfer encoding
      "([0-9a-fA-F]+\\r\\n)",       // Chunk size
      "(\\r\\n)"                    // New line between chunks
    }), ({ 1, 2, 3 }));

    parts = assoc[0];
    indices = assoc[1];

    total_size = 0;
    for(i = 0, sz = sizeof(parts); i < sz; i++) {
      // Regular old stuff. Nothing to see here.
      if(indices[i] == 0)
        continue;

      switch(indices[i]) {
        case 1: // End of chunked transfer encoding
          end_found = 1 ; // We done here.
          break;
        case 2: { // Get the chunk size and add it to the total size
          int chunk_size;
          sscanf(parts[i], "%x", chunk_size);
          total_size += chunk_size;
          break;
        }
        case 3: // New line between chunks
          break ; // will be replaced with "" after this switch
      }

      parts[i] = "";
    }

    response = implode(parts, "");

    _log(3, "Total size of chunked response: %d", total_size);
    _log(3, "Size of response: %d", sizeof(buf));
  }
  // Content-Length
  else if(server["response"]["headers"]["content-length"]) {
    int expected = server["response"]["headers"]["content-length"];
    int sz;

    _log(2, "Content-Length: %d", expected);

    cache = read_cache(file);
    buf = to_string(cache);
    sz = sizeof(cache);

    _log(3, "Do we have any new lines to strip? %d", pcre_match(buf, rn));
    if(pcre_match(buf, rn)) {
      _log(2, "Size of response before removing new lines: %d", sz);
      while(pcre_match(buf, rn)) {
        _log(2, "> Substituting new line with empty string");
        buf = pcre_replace(buf, capture, ({ replace }));
      }
    }

    if(sz == expected) {
      _log(2, "Content-Length: %d", expected);
      _log(2, "Size of response after removing new lines: %d", sizeof(cache));
      call_if(this_object(), "http_handle_response", server);
    } else
      _log(2, "Size of response (%d) does not match Content-Length (%d)", sizeof(buf), expected);

    response = buf;
  }
  // No content-length or chunked transfer encoding found. What is it???
  // If someone identifies the format, please update this!
  else {
    _log(2, "No content-length or chunked transfer encoding found. "
      "Therefore, assumingthe response is the entire body. 🤷🏻");

    response = buf;
  }

  _log(2, "Removing cache file: %s", file);

  err = catch {
    int result = rm(file);

    if(result == 0) {
      _log(2, "Failed to remove cache file: %s", file);
      throw("Failed to remove cache file: " + file);
    }
  };

  server["response"]["body"] = response;
  servers[fd] = server;
}

private nomask void process_redirect(int fd, mapping server) {
  mapping processed_url;
  string location;
  mapping headers;

  _log(2, "Processing redirect");

  location = server["response"]["headers"]["location"];
  if(!stringp(location) || !strlen(location)) {
    _log(0, "No location header found in redirect response");
    return;
  }

  processed_url = parse_url(location);

  if(!processed_url) {
    _log(0, "Failed to parse URL: %s", location);
    shutdown_socket(fd);
    return;
  }

  shutdown_socket(fd);

  _log(1, "Redirecting to: %s", location);

  headers = server["headers"];
  if(!headers)
    headers = ([]);

  headers["X-Redirect-Count"] = server["redirects"] + 1;

  http_request(
    location,
    server["request"]["method"],
    headers,
    server["request"]["body"],
  );
}

private nomask mixed send_http_request(int fd, mapping server) {
  string request, body;
  mapping headers = server["request"]["headers"];
  string method = server["request"]["method"];
  string path = server["request"]["path"];
  string host = server["request"]["host"];
  int result;

  _log(2, "Sending request: %s %s %s", method, path, host);

  if(!servers[fd])
    return "No such connection: " + fd;

  if(!stringp(method) || !stringp(path) || !stringp(host)) {
    _log(2, "Invalid request parameters");
    return "Invalid request parameters";
  }
  method = upper_case(method);

  request = method + " " + path + " HTTP/1.1\r\n";
  request += "Host: " + host + "\r\n";

  headers = headers || ([ ]);
  headers += default_headers;

  foreach(string key, string value in headers) {
    if(key != "Content-Type" && key != "Content-Length")
      request += key + ": " + value + "\r\n";
  }

  body = server["request"]["body"];

  if(body) {
    request += "Content-Length: " + strlen(body) + "\r\n";
    if(headers["Content-Type"]) {
      string content_type = headers["Content-Type"];
      string *matches, type, subtype, charset;

      matches = pcre_extract(
        content_type,
        "(\\w+)/([-\\w]+)(?:\\s*;\\s*charset=(\\S+))?"
      );

      switch(sizeof(matches)) {
        case 2:
          type = matches[0];
          subtype = matches[1];
          charset = "utf-8";
          break;
        case 3:
          type = matches[0];
          subtype = matches[1];
          charset = matches[2] || "utf-8";
          break;
        default:
          _log(2, "Failed to parse Content-Type header: %s", content_type);
          shutdown_socket(fd);
          return "Failed to parse Content-Type header: " + content_type;
      }

      request += sprintf("Content-Type: %s/%s; charset=%s\r\n", type, subtype, charset);
    }
  }

  request += "\r\n";

  if(body)
    request += body;

  result = socket_write(fd, request);

  if(result != EESUCCESS) {
    _log(3, "Failed to send request: %s", socket_error(result));
    shutdown_socket(fd);
    return "Failed to send request: " + socket_error(result);
  }

  _log(3, "Request sent: %s", identify(request));

  return result;
}

public nomask int set_max_redirects(int max) {
  max_redirects = max;
  return max_redirects;
}

public nomask int query_max_redirects() {
  return max_redirects;
}
