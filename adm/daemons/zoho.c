/**
 * @file /adm/daemons/zoho.c
 * @description This daemon is used to interact with the Zoho API.
 *
 * @created 2024-07-25 - Gesslar
 * @last_modified 2024-07-25 - Gesslar
 *
 * @history
 * 2024-07-25 - Gesslar - Created
 */

inherit STD_HTTP_CLIENT;

void send_email_with_token(string to, string subject, string body, mixed *cb, mapping token_result);
void get_valid_token(mapping args);
void handle_new_token(mixed *cb, mapping response);
void handle_email_result(mapping result);
void get_initial_tokens(mixed *cb);

private nosave nomask mapping config = json_decode(read_file("/adm/etc/secret/zoho.json"));
private mapping token_cache = ([]);
private nosave mapping call = ([]);

void setup() {
  set_persistent(1);
  set_log_prefix("(Zoho)");
  set_log_level(0);
}

// Function to get initial access and refresh tokens
void get_initial_tokens(mixed *cb) {
  string url = "https://accounts.zoho.com/oauth/v2/token";
  mapping post_data = ([
    "code": config["code"],
    "client_id": config["client_id"],
    "client_secret": config["client_secret"],
    "grant_type": config["grant_type"],
    "account_id": config["account_id"],
  ]);
  mapping headers = ([
    "Content-Type": "application/x-www-form-urlencoded"
  ]);
  mapping response;
  float request_id;
  string body;

  body = "";
  foreach(string key, mixed value in post_data)
    body += url_encode(key) + "=" + url_encode(value) + "&";

  response = http_request(url, "POST", headers, body);

  _log(3, "Response: %O", response);

  if(!mapp(response)) {
    _log(1, "Failed to initiate request for initial tokens");
    call_back(cb, (["error": "Failed to initiate request"]));
    return;
  }

  request_id = response["start_time"];
  if(request_id) {
    call[request_id] = ([
      "status": "get_initial_tokens",
      "callback": cb
    ]);
  } else {
    _log(1, "Failed to initiate request for initial tokens");
    call_back(cb, (["error": "Failed to initiate request"]));
  }
}

// Function to initiate getting a new access token
void get_new_access_token(mixed *cb) {
  string url = "https://accounts.zoho.com/oauth/v2/token";
  mapping post_data = ([
    "refresh_token": token_cache["refresh_token"],
    "client_id": config["client_id"],
    "client_secret": config["client_secret"],
    "grant_type": "refresh_token"
  ]);
  mapping headers = ([
    "Content-Type": "application/x-www-form-urlencoded"
  ]);
  string body;
  mapping response;
  float request_id;

  body = "";
  foreach(string key, mixed value in post_data)
    body += url_encode(key) + "=" + url_encode(value) + "&";

  response = http_request(url, "POST", headers, body);
  if(!mapp(response)) {
    _log(1, "Failed to initiate request for new access token");
    call_back(cb, (["error": "Failed to initiate request"]));
    return;
  }

  request_id = response["request"]["start_time"];

  if(request_id) {
    call[request_id] = ([
      "status": "get_new_access_token",
      "callback": cb
    ]);
  } else {
    _log(1, "Failed to initiate request for new access token");
    call_back(cb, (["error": "Failed to initiate request"]));
  }
}

// Function to send an email (initiates the process)
void send_email(string to, string subject, string body, mixed *cb) {
  get_valid_token(([
    "callback": assemble_call_back(this_object(), "send_email_with_token", to, subject, body, cb)
  ]));
}

// Function to actually send the email once we have a valid token
void send_email_with_token(mapping token_result, string to, string subject, string body, mixed *cb) {
  string url;
  mapping headers, response;
  mapping data;
  int request_id;

  _log(2, "Token result: %O", token_result);

  if(token_result["error"]) {
    _log(1, "Failed to get a valid token");
    call_back(cb, token_result);
    return;
  }

  url = "https://mail.zoho.com/api/accounts/" + config["account_id"] + "/messages";
  headers = ([
    "Authorization": "Zoho-oauthtoken " + token_result["access_token"],
    "Content-Type": "application/json"
  ]);
  data = ([
    "fromAddress": config["from_address"],
    "toAddress": to,
    "subject": subject,
    "content": body
  ]);

  response = http_request(url, "POST", headers, json_encode(data));

  _log(2, "Response: %O", response);

  if(!mapp(response)) {
    _log(1, "Failed to initiate email send request");
    call_back(cb, (["error": "Failed to initiate email send request"]));
    return;
  }

  request_id = response["start_time"];

  if(request_id) {
    call[request_id] = ([
      "status": "send_email",
      "callback": cb
    ]);
  } else {
    _log(1, "Failed to initiate email send request");
    call_back(cb, (["error": "Failed to initiate email send request"]));
  }
}

// Function to get a valid token (checks cache, refreshes if needed)
void get_valid_token(mapping args) {
  int current_time = time();

  if(token_cache["access_token"] && token_cache["expires_at"] > (current_time + 300)) {
    call_back(args["callback"], (["access_token": token_cache["access_token"]]));
    return;
  }

  if(!token_cache["refresh_token"])
    get_initial_tokens(assemble_call_back(this_object(), "handle_new_token", args["callback"]));
  else
    get_new_access_token(assemble_call_back(this_object(), "handle_new_token", args["callback"]));
}

// Handler for new token response
void handle_new_token(mapping response, mixed *cb) {
  _log(3, "New token response: %O", response);

  if(response["error"]) {
    call_back(cb, response);
    return;
  }

  token_cache = ([
    "access_token": response["access_token"],
    "refresh_token": response["refresh_token"] || token_cache["refresh_token"],
    "expires_at": time() + to_int(response["expires_in"])
  ]);

  saveDatda();

  call_back(cb, (["access_token": response["access_token"]]));
}

nomask void http_handle_shutdown(mapping data) {
  int incoming_serial = data["request"]["start_time"];
  mapping curr = call[incoming_serial];
  mapping response;
  mapping result;
  mapping body;

  if(!mapp(curr))
    return;

  _log(4, "Data: %O", data);

  response = data["response"];
  body = json_decode(response["body"]);

  _log(5, "Body: %O", body);

  switch(curr["status"]) {
    case "get_initial_tokens":
    case "get_new_access_token":
      if(response["status"]["code"] == 200) {
        if(body["error"]) {
          switch(curr["status"]) {
            case "get_initial_tokens":
              result = (["status": "error", "error": "Failed to get initial token. Error: " + body["error"]]);
              break;
            case "get_new_access_token":
              result = (["status": "error", "error": "Failed to get/refresh token. Error: " + body["error"]]);
              break;
          }
        } else {
          result = ([
            "status": "success",
            "access_token": body["access_token"],
            "refresh_token": body["refresh_token"],
            "expires_in": body["expires_in"]
          ]);
        }
      } else
        result = (["status": "error", "error": "Failed to get/refresh token. Status: " + response["status"]["code"]]);
      break;
    case "send_email":
      if(response["status"]["code"] == 200)
        result = (["status": "success", "success": "Email sent successfully"]);
      else
        result = (["status": "error", "error": "Failed to send email. Status: " + response["status"]["code"]]);
      break;
  }

  call_back(curr["callback"], result);

  map_delete(call, incoming_serial);
}
