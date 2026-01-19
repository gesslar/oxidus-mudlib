/**
 * @file /adm/daemons/github_issues.c
 * @description Daemon to handle github issues
 * @details This daemon is used to handle GitHub issues for the mud. It uses
 * the GitHub Issues API to create issues in the mud's GitHub repository.
 *
 * @created 2024-07-07 - Gesslar
 * @last_modified 2024-07-07 - Gesslar
 *
 * @history
 * 2024-07-07 - Gesslar - Created
 */

#include <daemons.h>

inherit STD_HTTP_CLIENT;

private nomask int parse_time(string str);
private nomask int is_setup();
private nomask void check_token();
nomask void process_backlog();
private nomask void process_next(string *files);
private nomask void execute_callback(mapping request, mapping response);

private nomask nosave mapping api_calls = ([ ]);

void setup() {
  set_log_level(4);
  set_log_prefix("(GITHUB_ISSUES)");

  assure_dir("/data/github/issues/pending");

  process_backlog();
}

/**
 * @daemon_function create_issue
 * @description Create a new issue in the GitHub repository
 * @param {string} type - The type of issue to create
 * @param {string} title - The title of the issue
 * @param {string} body - The body of the issue
 * @param {mixed} callback - The callback function to call when the request is
 *                           complete
 * @returns mixed - Returns 1 if the request was successful, a string if there
 *                 was an error.
*/

varargs mixed create_issue(string type, string title, string body, mixed *callback) {
  mapping config = mudConfig("GITHUB_REPORTER");

  if(!stringp(type) || !strlen(type))
    return "Type must be a valid string";

  if(member_array(type, config["types"]) == -1)
    return "Invalid type. Available types: " + implode(config["types"], ", ");

  if(!stringp(title))
    return "Title must be a string";

  if(!stringp(body))
    return "Body must be a string";

  if(!config["owner"] || !config["repo"] || !config["token"])
    return "GitHub Issues API not setup in adm/etc/config.json";

  string url = sprintf(
      "https://api.github.com/repos/%s/%s/issues",
      config["owner"], config["repo"]
    );

  mapping request = http_request(
    url,
    "POST",
    ([
      "Authorization": sprintf("token %s", config["token"]),
      "Accept": "application/vnd.github.v3+json",
      "Content-Type": "application/json; charset=utf-8"
    ]),
    json_encode(([
      "title": title,
      "body": "```\n" + body + "\n```\n",
      "labels": ({ type })
    ])),
  );

  api_calls[request["start_time"]] = ([
    "type" : "create",
    "label" : type,
    "title" : title,
    "body" : body,
    "callback" : callback,
    "caller" : previous_object(),
    "request" : request
  ]);

  return mapp(request) ? 1 : 0;
}

/**
 * @function http_handle_shutdown
 * @description Handle the response from the GitHub API
 * @param {mapping} response - The response from the GitHub API
*/
void http_handle_shutdown(mapping response) {
  mixed body = response["response"]["body"];
  int request_id = response["request"]["start_time"];
  mapping request;
  if(!request = api_calls[request_id]) {
    _log(2, "No request found for id %d", request_id);
    return;
  }

  map_delete(api_calls, request_id);

  if(response["response"]["status"]["code"] != 201) {
    string file;
    _log(2, "Request error: %d %s\nO",
      response["response"]["status"]["code"],
      response["response"]["status"]["message"],
      body
    );
    file = time_ns() + ".txt";
    write_file("/data/github/issues/pending/" + file, json_encode(request));
    _log(2, "Request saved to /data/github/pending/%s", file);
    return;
  }

  _log(2, "Content-Type: %O", response["response"]["headers"]["content-type"]);

  body = parse_body(body, response["response"]["headers"]["content-type"]);

  string err = catch (call_back(request["callback"], response["response"]["status"]));
  if(err)
    _log(2, "Error calling callback: %O", err);
}
// TODO: fix up to use assemble_call_back and call_back
/**
 * @function execute_callback
 * @description Execute the callback function
 * @param {mapping} request - The request containing the callback function
 * @param {mapping} response - The response from the GitHub API
*/
void execute_callback(mapping request, mapping response) {
  string err = catch {
    if(request["callback"]) {
      if(stringp(request["callback"]) && request["callback"] != "RESUBMIT") {
        if(objectp(request["caller"]) && function_exists(request["callback"], request["caller"])) {
          call_other(request["caller"], request["callback"], response);
        } else {
          _log(2, "Invalid caller or callback function: %O %O", request["caller"], request["callback"]);
        }
      } else if(valid_function(request["callback"])) {
        function f = request["callback"];

        f(response);
      } else {
        _log(2, "Invalid callback function: %O", request["callback"]);
      }
    }
  };

  if(err)
    _log(2, "Error calling callback: %O", err);
}

/**
 * @daemon_function process_backlog
 * @description Process the backlog of requests
 * @details This function processes the backlog of requests in the
 *          /data/github/issues/pending directory. It passes a list of files to
 *          process_next() to process each request.
*/
public nomask void process_backlog() {
  string *files = get_dir("/data/github/issues/pending/*.txt");

  if(!sizeof(files))
      return;

  process_next(files);
}

/**
 * @function process_next
 * @description Process the next request in the backlog
 * @param {string[]} files - The list of files remaining in the backlog to process
*/
private nomask void process_next(string *files) {
  function schedule_next = (: call_out_walltime("process_next", 5.0, $1) :);

  if(!sizeof(files))
    return;

  string file = files[0];

  if(!file_exists(file)) {
    if(sizeof(files) > 1)
      schedule_next(files[1..]);
    else
    return;
}

  mapping request;
  string err = catch(request = json_decode(read_file("/data/github/issues/pending/" + file)));
  if(err) {
    _log(2, "Error reading request file %s: %O", file, err);

    if(sizeof(files) > 1)
      schedule_next(files[1..]);

    return;
  }

  if(!mapp(request)) {
    _log(2, "Invalid request in /data/github/issues/pending/%s", file);

    if(sizeof(files) > 1)
      schedule_next(files[1..]);

    return;
  }

  if(!mapp(request["request"])) {
    _log(2, "Invalid request in /data/github/issues/pending/%s", file);
    return;
  }

  _log(2, "Processing request from /data/github/issues/pending/%s", file);
  _log(2, "Request: %O", request);
  _log(2, "Resubmitting request from /data/github/issues/pending/%s", file);

  // Do appropriate function based on its type
  err = catch {
    switch(request["type"]) {
      case "create": {
        mixed *new_request = ({
          request["label"],
          request["title"],
          request["body"],
          request["callback"]
        });

        create_issue(new_request...);
        _log(2, "Submitting create issue request: %O", new_request);
        break;
      }
      default:
        _log(2, "Invalid request type: %s", request["type"]);
        break;
    }
  };

  if(err) {
      _log(2, "Error sending request: %O", err);
  } else {
    rm("/data/github/issues/pending/" + file);

    if(sizeof(files) > 1)
      schedule_next(files[1..]);

    _log(2, "Request resubmitted for /data/github/issues/pending/%s", file);
  }
}
/**
 * @function parse_time
 * @description convert a GitHub date-time string to an epoch time
 * @param {string} datetime - The date-time string to convert
 * @returns {int} - The epoch time
 */
private nomask int parse_time(string datetime) {
    // Convert the date-time string to a time structure using strptime
  int epoch_time;
  string err = catch(epoch_time = strptime("%Y-%m-%dT%H:%M:%SZ", datetime));
  if(err) {
    _log(0, "Error parsing date-time string %s: %O", datetime, err);
    return -1;
  }

  // If strptime returns -1, it means the date-time string is invalid
  if(!epoch_time)
    error("Invalid date-time string");

  return epoch_time;
}
