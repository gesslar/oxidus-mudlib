/**
 * @file /std/modules/http.c
 * @description HTTP module with shared functions for use in HTTP operations
 *
 * @created 2024-07-05 - Gesslar
 * @last_modified 2024-12-07 - Gesslar
 *
 * @history
 * 2024-07-05 - Gesslar - Created
 * 2024-12-07 - Gesslar - Refactor HTTP module: Improve logging and buffer
 *                        handling in cache response and read cache functions.
 *                        Added integrity check for cached responses and
 *                        enhanced error handling for JSON decoding. Cleaned
 *                        up code for better readability.
 */

inherit M_LOG;

#include <daemons.h>
#include <localtime.h>
#include <http.h>

protected nomask string url_encode(string str);
protected nomask string url_decode(string str);

protected nomask buffer to_binary(string str);
protected nomask string to_string(buffer buf);

protected nomask varargs int find_marker(mixed buf, mixed marker);
protected nomask varargs int bufsrch(buffer buf, mixed str);
protected nomask void cache_response(string file, mixed response);
protected nomask mixed read_cache(string file);
protected nomask void set_option(string key, mixed value);
protected nomask mixed get_option(string key);

nomask mapping parse_http_request(buffer buf);
nomask mapping parse_http_response(string str);
nomask mapping parse_http_request_line(string str);
nomask mapping parse_headers(string str, int keep_remainder);
nomask mapping parse_body(string str, string content_type);
nomask mapping parse_route(string str);
nomask mapping parse_query(string str);
nomask mapping parse_url(string str);

private nomask nosave mapping options = ([]);

protected nomask mapping parse_http_request(buffer buf) {
  string str = to_string(buf);
  mapping result = ([ ]);
  string *matches, content_type;

  // We need the first line that contains the request method, path, and
  // version. And capture the headers string and the body.

  // First, we'll get the first line that contains the request method, path,
  matches = pcre_extract(str,
    "^((?:GET|POST|PUT|DELETE|OPTIONS|HEAD|PATCH|TRACE|CONNECT)\\s+" // Method
    "\\S+\\s+" // Path
    "HTTP/\\d\\.\\d)\\r\\n" // Version
    "([\\s\\S]*)$" // Rest of the request
  );

  // We didn't find the request line
  if(!sizeof(matches)) {
    result["buffer"] = buf;
    return result;
  }

  _log(3, "Request line string: %s", matches[0]);

  result["request"] = parse_http_request_line(matches[0]);
  _log(3, "Request line: %O", result["request"]);
  if(!result["request"]) return 0;

  result["route"] = parse_route(result["request"]["path"]);
  _log(3, "Request route: %O", result["route"]);
  if(!result["route"]) return 0;

  // Next, we'll get the headers string

  // We didn't find the headers string
  if(sizeof(matches) < 2) {
    result["buffer"] = to_binary(str);
    return result;
  }

  str = matches[1];

  matches = pcre_extract(str, "^([\\s\\S]*)\\r\\n\\r\\n([\\s\\S]*)$");
  // We didn't find the headers
  if(!sizeof(matches)) {
    result["buffer"] = to_binary(str);
    return result;
  }

  // We found the headers
  result["headers"] = parse_headers(matches[0], 0);
  _log(3, "Request headers: %O", result["headers"]);
  if(!result["headers"]) return 0;

  // Now we'll get the body
  if(sizeof(matches) != 2) {
    result["buffer"] = to_binary(str);
    return result;
  }

  str = matches[1];

  content_type = result["headers"]["content-type"];
  result["body"] = parse_body(str, content_type);
  _log(3, "Request body: %O", result["body"]);

  return result;
}

protected nomask mapping parse_http_response(string str) {
  string header, body;
  mapping result = ([]);
  int start_cursor, end_cursor;

  start_cursor = 0;
  end_cursor = find_marker(str, "\r\n\r\n");
  if(end_cursor == -1)
    return 0;

  header = str[0..end_cursor-2];
  body = str[end_cursor+2..];

  _log(1, "header: %s", header);

  result["headers"] = parse_headers(header, 0);
  if(!result["headers"]) return 0;

  result["body"] = parse_body(body, result["headers"]["content-type"]);
  if(!result["body"]) return 0;

  return result;
}

protected nomask mapping parse_http_request_line(string request_line) {
  string *parts;
  mapping result = ([]);

  // Define the regex pattern for matching the HTTP method, path, and version
  string pattern = "^(GET|POST|PUT|DELETE|OPTIONS|HEAD|PATCH|TRACE|CONNECT)\\s+([^\\s]+)\\s+HTTP/(\\d\\.\\d)$";

  // Extract the method, path, and version using regex
  parts = pcre_extract(request_line, pattern);

  // Ensure parts were extracted correctly
  if(sizeof(parts) == 3) {
    result["method"] = parts[0];
    result["path"] = parts[1];
    result["version"] = parts[2];
  } else
    return 0;

  return result;
}

protected nomask mapping parse_route(string route) {
  mapping result = ([]);
  string *parts;

  // Define the regex pattern for matching the path and optional query
  string pattern = "^(/[^?]*)(?:\\?(.*))?$";

  // Extract the path and query using regex
  parts = pcre_extract(route, pattern);

  // Ensure parts were extracted correctly
  if(sizeof(parts) >= 1) { // parts[0] is the path, parts[1] is the query if present
    result["route"] = parts[0];
    if(sizeof(parts) == 2) {
      mapping query = parse_query(parts[1]);
      result["query"] = query;
    } else
      result["query"] = "";
  } else
    throw("Bad query.");

  return result;
}

protected nomask mapping parse_response_status(mixed str, int keep_remainder: (: 0 :)) {
  string *parts;
  mapping result = ([]);
  int sz;

  // Define the regex pattern for matching the HTTP status code and message
  string pattern = "^HTTP/\\d(?:\\.\\d)?\\s+(\\d{3})(?: )?([\\w ]+)?\\r\\n([\\s\\S]+)$";

  if(bufferp(str))
    str = to_string(str);

  _log(4, "Testing string: %s", str);

  // Extract the status code, message, and remainder using regex
  parts = pcre_extract(str, pattern);

  _log(4, "parts: %O", parts);

  // Ensure parts were extracted correctly
  sz = sizeof(parts);
  if(!sz)
    return 0;


  result["code"] = to_int(parts[0]);
  if(sz >= 2)
    result["message"] = parts[1];
  else
    result["message"] = "";

  if(keep_remainder && sz == 3)
    result["buffer"] = to_binary(parts[2]);

  return result;
}

protected nomask mapping parse_headers(mixed str, int keep_remainder) {
  mapping headers = ([]);
  string search_pat = "^.*?:\\s+.*\r\n";
  string extract_pat = "^(.*?):\\s+(.*)\r\n";
  string replace_pat = "^(.*?:\\s+.*\r\n)";
  string *match;

  if(bufferp(str))
    str = to_string(str);

  while(pcre_match(str, search_pat)) {
    match = pcre_extract(str, extract_pat);

    if(sizeof(match) >= 2) {
      string header_name = lower_case(match[0]);
      string header_value = match[1];

      if(pcre_match(header_value, "^\\d+$"))
        headers[header_name] = to_int(header_value);
      else if(pcre_match(header_value, "^\\d+\\.\\d+$"))
        headers[header_name] = to_float(header_value);
      else {
        switch(header_name) {
          case "connection":
          case "accept-encoding":
            headers[header_name] = explode(header_value, ",");
            headers[header_name] = map(headers[header_name], (: trim :));
            break;
          default:
            headers[header_name] = header_value;
            break;
        }
      }

      str = pcre_replace(str, replace_pat, ({ "" }));
    } else
      break;
  }

  if(keep_remainder)
    headers["buffer"] = to_binary(str)[2..];

  return headers;
}

protected nomask mapping parse_query(string str) {
  mapping payload = ([]);
  string *args;

  args = explode(str, "&");

  if(sizeof(args)) {
    foreach(string arg in args) {
      string *parts;

      parts = explode(arg, "=");
      if(sizeof(parts) == 2)
        payload[url_decode(parts[0])] = url_decode(parts[1]);
    }
  }

  return payload;
}

protected nomask mixed parse_body(mixed body, string content_type) {
  mixed payload = ([]);
  string type, encoding;
  string *encoding_matches;
  mixed err;

  _log(2, "Beginning parse_body");

  if(!body || body == "")
    return 0;

  if(bufferp(body))
    body = to_string(body);

  if(!content_type)
    content_type = "auto";

  _log(2, "Content-Type: %s", content_type);

  if(!sizeof(encoding_matches = pcre_extract(content_type, "([^;]+)(?:;\\s*charset=(.*))?"))) {
    _log(2, "Failed to extract encoding from content-type: %s", content_type);
    return body;
  }

  type = encoding_matches[0];
  if(sizeof(encoding_matches) > 1)
    encoding = encoding_matches[1];

  _log(2, "Parsing body of type %s", type);
  switch (type) {
    case CONTENT_TYPE_APPLICATION_JSON:
      // Attempt to decode JSON payload
      // Use regex to check if the body is a JSON object
      if(pcre_match(body, "^\\s*\\{[\\s\\S]*\\}\\s*$") == 1) {
        err = catch(payload = json_decode(body));
        if(err) {
          _log(1, "Failed to decode JSON payload: %O", err);
          payload = body;
        } else {
          _log(3, "Decoded JSON payload: %O", payload);
        }
      }
      break;
    case CONTENT_TYPE_APPLICATION_FORM_URLENCODED: {
      // Decode URL-encoded form data
      string *args;
      args = explode(body, "&");
      if(sizeof(args)) {
        payload = ([]);
        foreach(string arg in args) {
          string *parts;
          parts = explode(arg, "=");
          if(sizeof(parts) == 2)
            payload[url_decode(parts[0])] = url_decode(parts[1]);
        }
      }
      _log(3, "Decoded form data: %O", payload);
      break;
    }
    case "auto" : {
      string *matches;

      if(sizeof(matches = pcre_extract(body, "^\\s*(\\{[\\s\\S]*\\})\\s*$"))) {
        body = matches[0];
        err = catch(payload = json_decode(body));
        if(err) {
          _log(2, "Failed to decode JSON payload: %O", err);
          payload = body;
        } else
          _log(3, "Decoded JSON payload: %O", payload);
      } else {
        payload = body;
        _log(3, "Parsed text body: %s", payload);
      }
      break;
    }
    default:
      // Just send it back
      _log(3, "Unknown content type: %s", type);
      payload = body;
      break;
  }

  _log(2, "Sizeof payload: %d", sizeof(payload));

  return payload;
}

protected nomask string url_encode(string str) {
  string allowed = "-_.~";
  string result = "";
  int sz, i;

  sz = sizeof(str);
  for(i = 0; i < sz; i++) {
    if(pcre_match(str[i..i], "^[a-zA-Z0-9" + allowed + "]$") == 0)
      result += sprintf("%%%02X", str[i]);
    else
      result += str[i..i];
  }

  return result;
}

protected nomask string url_decode(string str) {
  mixed *arrs;
  string *parts;
  int *matches;
  int sz, i;

  // Use pcre_assoc to find percent-encoded sequences
  arrs = pcre_assoc(str, ({ "%[A-Fa-f0-9]{2}" }), ({ 1 }));

  parts = arrs[0];
  matches = arrs[1];

  sz = sizeof(parts);
  for(i = 0; i < sz; i++) {
    if(matches[i] == 1) {
      int char;
      sscanf(parts[i], "%%%x", char);
      parts[i] = sprintf("%c", char);
    }
  }

  return implode(parts, "");
}

protected nomask mapping parse_url(string str) {
  string protocol, host, path;
  int port, secure;
  string *matches, port_string;
  mapping result = ([]);

  // Use regex to parse the URL and determine the protoco
  matches = pcre_extract(str, "^(http|ws)(s?)://([^:/]+):?(\\d+)?(/.*)?$");
  if(!sizeof(matches))
    return 0;

  protocol = matches[0];
  secure = matches[1] == "s";
  host = matches[2];
  if(sizeof(matches) >= 4)
    port_string = matches[3];
  if(sizeof(matches) >= 5)
    path = matches[4];

  if(!strlen(port_string))
    port = secure ? 443 : 80;
  else
    port = to_int(port_string);

  result = ([
    "protocol" : protocol + (secure ? "s" : ""),
    "secure" : secure,
    "host" : host,
    "port" : port,
    "path" : path || "/",
  ]);

  return result;
}

protected nomask varargs int find_marker(mixed buf, string marker) {
  int sz;
  string type = typeof(buf);

  sz = sizeof(buf);
  if(stringp(buf))
    buf = to_binary(buf);

  if(bufferp(buf))
    return bufsrch(buf, to_binary(marker));

  error("Invalid buffer type: " + type);
}

protected nomask varargs int bufsrch(buffer buf, mixed str) {
  int buf_sz;
  int sub_buf_sz;
  mixed sub_buf;
  int start;

  buf_sz = sizeof(buf);
  if(stringp(str))
    sub_buf = to_binary(str);
  else
    sub_buf = str;

  sub_buf_sz = sizeof(sub_buf);
  start = 0;

  if(sub_buf_sz > buf_sz || start < 0)
    return -1;

  for(int i = start; i <= buf_sz - sub_buf_sz; i++) {
    int match = 1;
    for(int j = 0; j < sub_buf_sz; j++) {
      if(buf[i+j] != sub_buf[j]) {
        match = 0;
        break;
      }
    }
    printf("Debug: Checking at index %d, match = %d\n", i, match);
    if(match)
      return i;
  }

  return -1;
}

protected nomask string binary_to_hex(buffer buf) {
  string hex = "";
  int len = sizeof(buf);

  _log(4, "Buffer = %O", buf);

  for(int i = 0; i < len; i++)
    hex += sprintf("0x%02x ", buf[i]);

  // Remove the trailing and space
  if(len > 0)
    hex = trim(hex);

  _log(3, "Converted %d bytes to hex", sizeof(buf));
  return hex;
}

protected nomask buffer hex_to_binary(string hex) {
  buffer binary = allocate_buffer(strlen(hex) / 2);
  int byte;

  for(int i = 0; i < strlen(hex); i += 2) {
    sscanf(hex[i..i+1], "%x", byte);
    binary[i / 2] = byte;
  }

  return binary;
}

protected nomask void cache_response(string file, buffer response) {
  int fs = file_size(file); // Current file size
  int sz, result;
  int start = fs > 0 ? fs : 0; // Start at the end of the file if it exists, otherwise at 0

  _log(3, "Writing to cache file: " + file);
  _log(3, "Filename " + file + " exists (size: %d): %s", fs, (fs > 0 ? "true" : "false"));

  // Ensure the response is a buffer
  if(!bufferp(response)) {
    _log(4, "Response is not a buffer: %O", response);
    return;
  }

  sz = sizeof(response);

  // Write the buffer to the file
  result = write_buffer(file, start, response);
  if(result != sz) {
    _log(4, "Failed to write entire buffer to file: %s (expected %d bytes, wrote %d bytes)", file, sz, result);
    return;
  }

  _log(3, "Wrote %d bytes to file: %s, new file size: %d", sz, file, file_size(file));
}

private nomask int integrity_check(string file, buffer response) {
  buffer cached_content = (buffer)read_buffer(file, 0, file_size(file));
  if(sizeof(cached_content) != sizeof(response)) {
    _log(4, "Mismatch detected: Cached file size: %d, Response size: %d", sizeof(cached_content), sizeof(response));
    return 0;
  }

  for(int i = 0; i < min(({ sizeof(cached_content), sizeof(response) })); i++) {
    if(cached_content[i] != response[i]) {
      _log(4, "Discrepancy at byte %d: Cached file: %02x, Response: %02x", i, cached_content[i], response[i]);
      break;
    }
  }

  return 1;
}

protected nomask mixed read_cache(string file) {
  mixed response = "";
  int chunk_size = (int)get_config(__MAX_BYTE_TRANSFER__);
  int max = (int)get_config(__MAX_STRING_LENGTH__);
  buffer total;
  string tail;

  int curr = 0;
  int sz = file_size(file);
  int bytes_to_read;

  if(sz == -1) {
    _log(2, "Cache file does not exist: %s", file);
    return response;
  } else if(sz == 0) {
    _log(2, "Cache file is empty: %s", file);
    return response;
  }

  tail = tail(file, 5);
  _log(3, "Tail of file (5 lines): %O", tail);

  if(sz < 1)
    return response;

  if(sz > max)
    return 0;

  total = allocate_buffer(sz);

  while(curr < sz) {
    bytes_to_read = min(({sz - curr, chunk_size}));

    buffer buf = (buffer)read_buffer(file, curr, bytes_to_read);
    if(!buf) // Exit loop if read_bytes fails
      break;

    _log(4, "Read buffer: %s", to_string(buf));

    write_buffer(total, curr, buf);
    _log(4, "Accumulated response: %s", to_string(total));
    curr += sizeof(buf);
  }

  _log(3, "Last 10 bytes of response: %O", identify(total[<10..]));

  _log(3, "File size: %d", sz);
  _log(3, "Bytes read: %d", sizeof(total));
  _log(3, "Integrity check: %d", integrity_check(file, total));
  return total;
}

protected nomask string http_time_string() {
  int now = time();
  mixed *lt = localtime(now);
  int off = lt[LT_GMTOFF];

  now += off + (-1 * lt[LT_ISDST]) * 3600;
  return strftime("%a, %d %b %Y %H:%M:%S GMT", now);
}

protected nomask int http_content_length(mapping client) {
  mixed body;
  buffer buf;

  body = client["http"]["response"]["body"];

  if(!body)
    return 0;

  if(stringp(body))
    buf = to_binary(body);
  else if(bufferp(body))
    buf = body;

  if(!bufferp(buf))
    return -1;

  return sizeof(body);
}

protected nomask void set_option(string key, mixed value) {
  _log(3, "Setting option %s to %O", key, value);

  options[key] = value;
}

protected nomask mixed get_option(string key) {
  return options[key];
}

protected nomask buffer to_binary(string str) { return string_encode(str, "UTF-8"); }
protected nomask string to_string(buffer buf) { return string_decode(buf, "UTF-8"); }
