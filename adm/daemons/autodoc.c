/**
 * @file /adm/daemons/autodoc.c
 * @description Autodocumentation system that parses JSDoc-style comments in
 *              LPC source files and generates documentation in a structured
 *              format.
 *
 * @created 2024-02-18 - Gesslar
 * @last_modified 2024-07-19 - Gesslar
 *
 * @history
 * 2024-02-18 - Gesslar - Created
 * 2024-07-11 - Gesslar - Entirely rewritten from Lima-style parsing to JSDoc
 * 2024-07-19 - Gesslar - Added new tags to parse @def and now writes help files
 */

inherit STD_DAEMON;
inherit M_LOG;

// Function prototypes
private nomask int check_running();
public nomask mixed autodoc_scan();
private nomask void finish_scan();
private nomask void parse_file(string file);
private nomask mixed *consolidate_function(string function_name, mapping func);
private nomask string generate_function_markdown(string function_name, mapping func);

// Variables
private nosave nomask float dir_delay = 0.02;
private nosave nomask float file_delay = 0.01;

private nosave nomask float start_time = 0.0;

private nosave nomask string doc_root, wiki_doc_root;
private nosave nomask string *dirs_to_check = ({});
private nosave nomask string *files_to_check = ({});

private nosave nomask int scanning = null;
private nosave nomask int writing = null;

private nosave nomask string jsdoc_function_regex,
                            *jsdoc_function_ignore_tags,
                            *tags, tag_regex,
                             continue_regex,
                             blank_line_regex,
                             function_detect_regex,
                             jsdoc_array_regex,
                             * multi_line_tags;

private nosave nomask mapping docs = ([]);
private nosave int ci = false;
private nosave int total_dirs_scanned, total_files_scanned;

void setup() {
  set_log_level(1);

  jsdoc_function_regex = "^\\s\\*\\s+@(\\w+)\\s+(\\w+)\\s*$";
  jsdoc_array_regex = "\\w+(\\s*\\[\\s*\\]\\s*)";

  jsdoc_function_ignore_tags = ({ "function" });
  multi_line_tags = ({ "description", "example" });
  blank_line_regex = "^\\s\\*\\s*$";

  tags = ({ "description", "def", "param", "returns", "example" });
  tag_regex = sprintf("^\\s*\\*\\s+@(%s)\\s+(.*)$", implode(tags, "|"));

  continue_regex = "^\\s\\*\\s+(.*)$";

  function_detect_regex =
  "^(?:public|protected|private)?\\s*"
  "(?:nomask|varargs)?\\s*(?:nomask|varargs)?\\s*"
  "(?:int|float|void|string|object|mixed|mapping|array|buffer|function)\\s*"
  "\\*?\\s*([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\("
;
}

/**
 * @daemon_function autodoc_scan
 * @description Start the autodoc scan process. This will trigger the daemon to
 *              scan the directories specified in the AUTODOC_SOURCE_DIRS
 *              configuration variable for LPC source files and parse the
 *              JSDoc-style comments in those files.
 *
 *              The parsed documentation will be written to the
 *              AUTODOC_ROOT directory in a structured format for use in
 *              generating documentation for the mudlib.
 *
 *              The parsed documentation will also be written to the
 *              WIKI_DOC_ROOT directory in a structured format for use in
 *              generating documentation for the wiki.
 * @returns {mixed} - 1 if the scan was started successfully, an error message
 *                    if the scan is already running.
 */
public nomask mixed autodoc_scan() {
  if(check_running() == true)
      return "Autodoc is already running.";

  _log(1, "Starting autodoc scan");
  if(this_body() && devp(this_body()))
      _ok("Starting autodoc scan");

  writing = false;
  doc_root = mudConfig("AUTODOC_ROOT");
  wiki_doc_root = mudConfig("WIKI_DOC_ROOT");
  start_time = time_frac();
  dirs_to_check = mudConfig("AUTODOC_SOURCE_DIRS");
  scanning = sizeof(dirs_to_check);
  total_dirs_scanned = 0;
  total_files_scanned = 0;

  RECURSE_RMDIR_D->recurse_rmdir(doc_root);
  // RECURD_RMDIR_D->recurse_rmdir(wiki_doc_root);

  docs = ([]);
  files_to_check = ({});

  call_out_walltime("check_dir", dir_delay);

  return 1;
}

private nomask void check_dir() {
  string *files;
  string source_dir;

  scanning -= 1;
  total_dirs_scanned += 1;

  source_dir = dirs_to_check[0];
  source_dir = append(source_dir, "/");
  files = get_dir(append(source_dir, "*.c"));
  files = map(files, (: $2 + $1 :), source_dir);
  files_to_check += files;
  scanning += sizeof(files);

  dirs_to_check = dirs_to_check[1..];

  if(sizeof(dirs_to_check))
    // Keep checking dirs
    call_out_walltime("check_dir", dir_delay);
  else
    // No more dirs, let's start checking files
    call_out_walltime("check_file", file_delay);
}

private nomask void check_file() {
  string file;
  mixed err;

  scanning -= 1;
  file = files_to_check[0];
  files_to_check = files_to_check[1..];

  err = catch(parse_file(file));
  if(err)
    log_file("system/autodoc", "Error parsing file: " + err + "\n");
  else
    total_files_scanned += 1;

  if(!sizeof(files_to_check))
    return finish_scan();

  call_out_walltime("check_file", file_delay);
}

private nomask void parse_file(string file) {
  string *lines;
  int num, max;
  int in_jsdoc = 0;
  string doc_type, function_tag, line;
  int j;

  if(!file_exists(file))
    return;

  lines = explode(read_file(file), "\n");

  max = sizeof(lines);
  num = 0;

  for(; num < max; num++) {
    int jsdoc_function_found;
    string current_tag;
    mapping curr;
    string *matches;
    string *tag_data;
    int new_tag_found;

    line = lines[num];

    // Check for the start of a JSDoc-style comment
    if(!in_jsdoc) {
      // Clear stuff since we've definitely exited the JSDoc comment block
      if(stringp(function_tag)) {
        function_tag = null;
        jsdoc_function_found = 0;
        current_tag = null;
        tag_data = null;
      }

      if(pcre_match(line, "^/\\*\\*") == true) {
        in_jsdoc = 1;
        jsdoc_function_found = 0;
        current_tag = null;
        tag_data = null;
        curr = ([]); // Reset output buffer for new comment block
      } else
        continue;
    }

    // Process lines within the JSDoc comment block
    if(in_jsdoc == 1) {
      // If we encountered a new tag while iterating, we need to now save
      // the currently gathered information into curr([]);
      // And then keep on trucking!
      if(new_tag_found == 1) {
        curr = curr || ([]);
        tag_data = tag_data || ({});
        if(!of(current_tag, curr))
          curr[current_tag] = ({ tag_data });
        else
          // Append the new tag data to the existing tag data (if any
          curr[current_tag] += ({ tag_data });

        new_tag_found = 0;
        current_tag = null;
      }

      /* ************************************************************* */
      /* THIS IS THE END OF THE JSDOC COMMENT BLOCK                    */
      /* ************************************************************* */
      if(pcre_match(line, "^\\s*\\*/") == true) {
        in_jsdoc = 0;

        // Record the last tag data
        if(current_tag != null) {
          curr = curr || ([]);
          if(nullp(tag_data))
            tag_data = ({}); // Ensure we have an array to append to
          if(!of(current_tag, curr))
            curr[current_tag] = ({ tag_data });
          else
            // Append the new tag data to the existing tag data (if any
            curr[current_tag] += ({ tag_data });
        }

        /* ********************************************************* */
        /* NOW THAT WE HAVE FOUND THE END OF THE JSDOC COMMENT       */
        /* BLOCK, WE MUST LOOK FOR THE FUNCTION DEFINITION AND       */
        /* EXTRACT IT FOR OUR DOCUMENTATION.                         */
        /*                                                           */
        /* BUT ONLY IF WE DO NOT ALREADY HAVE A FUNCTION DEFINITION  */
        /* FROM PARAM TAGS.                                          */
        /* ********************************************************* */
        if(!of("def", curr)) {
          string function_def;
          // We need to find the function definition
          // We can start looking for the function definition
          // after the current line
          for(j = num + 1; j < max; j++) {
            line = lines[j];
            matches = pcre_extract(line, function_detect_regex);
            if(sizeof(matches) == 1) {
              function_def = matches[0];
              if(function_def == function_tag) {
                // Ensure the line ends with '{' and remove it
                line = trim(chop((line), "{", -1));
                curr["def"] = ({ ({ line }) });
                // Move the outer loop index to the function
                // definition line
                num = j;
                break;
              }
            }
          }

          // If a matching function is not found, skip to the next doc/
          // function check
          if(!of("def", curr)) {
            in_jsdoc = 0;
            continue;
          }
        }
        /* ********************************************************* */
        /* NOW THAT WE HAVE THE FUNCTION DEFINITION, WE CAN ADD THE  */
        /* DOCUMENTATION TO THE APPROPRIATE LOCATION IN THE docs([]) */
        /* ********************************************************* */
        if(!of(doc_type, docs))
          docs[doc_type] = ([]);

        curr["source_file"] = file;
        docs[doc_type][function_tag] = curr;
        in_jsdoc = 0;
        continue;
      } else {
        /* ********************************************************* */
        /* THIS IS THE BEGINNING OF THE JSDOC COMMENT BLOCK          */
        /* Format: @function_type function_tag                       */
        /* ********************************************************* */
        // Check for the first tag to determine the document type and
        if(!pcre_match(line, tag_regex) &&
            pcre_match(line, jsdoc_function_regex)) {
            matches = pcre_extract(line, jsdoc_function_regex);
            if(sizeof(matches) > 0) {
              /* ************************************************* */
              /* DOC_TYPE WILL BE THE CURRENT CATEGORY UNDER       */
              /* docs([]) UNLESS IT'S IN THE IGNORE LIST, THEN,    */
              /* NEVERMIND.                                        */
              /* ************************************************* */
              if(of(matches[0], jsdoc_function_ignore_tags)) {
                in_jsdoc = 0;
                continue;
              }

              doc_type = matches[0];
              function_tag = matches[1];
              // we can continue now and parse the rest in the next
              // iteration
              continue;
            }
          }

          /* ********************************************************* */
          /* THIS IS THE MIDDLE OF THE JSDOC COMMENT BLOCK             */
          /* WE ARE NOW LOOKING FOR A TAG                              */
          /* Format: @tag_name tag_info                                */
          /* WHERE tag_name IS ONE OF THE TAGS IN tags({})             */
          /* ********************************************************* */
          if(stringp(function_tag)) {
            // If no current tag is found, try to match a tag
            if(current_tag == null) {
              if(pcre_match(line, tag_regex)) {
                matches = pcre_extract(line, tag_regex);
                if(sizeof(matches) > 0) {
                  current_tag = matches[0];
                  tag_data = ({ matches[1] });
                }
              }
            /* ***************************************************** */
            /* WE HAVE A TAG! NOW WE NEED TO GET THE INFO PERTAINING */
            /* TO THAT TAG.                                          */
            /* ***************************************************** */
            } else {
              // It looks like we have encountered a new tag.
              // If so, we need to reset so we can get the new tag's
              // information.
              if(pcre_match(line, "^\\s*\\*\\s+@") == true) {
                // Reset the current tag and tag data
                new_tag_found = 1;
                num--;
                continue;
              }

              // If we have a blank line, we can reset the current
              // tag, we may have unexpectedly reached the end of the
              // JSDoc comment block.
              if(pcre_match(line, "^\\s*$") == true) {
                current_tag = null;
                tag_data = null;
                continue;
              }

              // If we haven't reached a new tag and we haven't
              // reached the end of the JSDoc comment block, we can
              // assume we are still parsing additional information
              // for the current tag. We can append the current line
              // to the current tag's information.
              if(pcre_match(rtrim(line), continue_regex)) {
                matches = pcre_extract(line, continue_regex);
                if(sizeof(matches) > 0)
                  tag_data += ({ matches[0] });
              } else if(of(current_tag, multi_line_tags)) {
                if(pcre_match(rtrim(line), blank_line_regex))
                  tag_data += ({ "" });
                else
                  tag_data += ({ trim(line) });
              } else
                tag_data += ({ trim(line) });
            }
          }
        }
    } else
      continue;
  }
}

// ({ function_name, *synopsis, *params, returns, description, *example })
private nomask mixed *consolidate_function(string function_name, mapping func) {
  mixed *result = allocate(6, 0);
  mixed *currs;
  string *curr;
  string line;
  mixed err;
  int sz;

  err = catch {
    // We need a description and a function definition to continue,
    // otherwise we can skip this function.
    if(!of("def", func) || !of("description", func))
      return result;

    // Add the function name [0]
    result[0] = function_name;

    // Add the synopsis [1]
    currs = func["def"];

    sz = sizeof(currs);
    result[1] = allocate(sz);
    while(sz--) {
      line = implode(currs[sz], " ");
      result[1][sz] = line;
    }

    // Add the parameters [2]
    if(of("param", func)) {
      currs = func["param"];
      sz = sizeof(currs);
      result[2] = allocate(sz);
      while(sz--) {
        string type, var, desc;

        line = implode(currs[sz], " ");
        if(sscanf(line, "{%s} %s - %s", type, var, desc) == 3) {
          while(pcre_match(type, jsdoc_array_regex))
            type = pcre_replace(type, jsdoc_array_regex, ({ "*" }));
          result[2][sz] = sprintf("* `%s %s` - %s", type, var, desc);
        } else
          result[2][sz] = line;
      }
    }

    // Add the returns [3]
    if(of("returns", func)) {
      string *parts;

      currs = func["returns"];
      curr = currs[0];
      line = implode(curr, " ");

      if(sizeof(parts = pcre_extract(line, "^\\{(.*)\\} - (.*)$")) == 2) {
        if(pcre_match(parts[0], jsdoc_array_regex))
          parts[0] = pcre_replace(parts[0], jsdoc_array_regex, ({ "*" }));
        result[3] = sprintf("`%s` - %s", parts[0], parts[1]);
      } else
      result[3] = line;
    }

    // Add the description
    currs = func["description"];
    curr = currs[0];
    line = implode(curr, "\n");
    result[4] = line;
  };

  if(err) {
    log_file("system/autodoc", "Error consolidating function: " + err + "\n");
    return ({ });
  }

  return result;
}

private nomask string generate_doc_content(string function_name, mapping func) {
  string out = "";
  mixed err;
  mixed *parts;

  err = catch(parts = consolidate_function(function_name, func));

  if(err) {
    log_file("system/autodoc", "Error generating document content: " + err + "\n");
    return null;
  }

  if(!sizeof(parts))
    return null;

  // ({ function_name, *synopsis, *params, returns, description, *example })

  // Add the synopsis
  out += implode(parts[1], "\n") + "\n";

  // Add the parameters
  if(sizeof(parts[2])) {
    string *param = parts[2];

    out += "\nParameters:\n\n" + implode(param, "\n") + "\n";
  }

  // Add the returns
  if(sizeof(parts[3]))
    out += "\nReturns\n\n" + parts[3] + "\n";

  // Add the description
  out += "\n" + parts[4] + "\n";

  return out;
}

private nomask void generate_mud_docs() {
  string *function_types, function_type;

  writing = true;

  function_types = keys(docs);

  foreach(function_type in function_types) {
    string *function_names, function_name;
    string dest_dir;
    mapping funcs;
    string *source_files, source_file;

    funcs = docs[function_type];
    function_names = sort_array(keys(funcs), 1);
    dest_dir = append(doc_root, function_type + "/");

    foreach(function_name in function_names) {
      mapping func;
      string doc_content;
      string dest_file = sprintf("%s%s", dest_dir, function_name);

      func = funcs[function_name];
      doc_content = generate_doc_content(function_name, func);

      if(!doc_content)
        continue;

      assure_file(dest_file);
      write_file(dest_file, doc_content, 1);
    }
  }
}

private nomask string generate_function_markdown(string function_name, mapping func) {
  string out = "";
  mixed err;
  mixed *parts;

  err = catch(parts = consolidate_function(function_name, func));

  if(err) {
    log_file("system/autodoc", "Error generating markdown: " + err + "\n");
    return null;
  }

  if(!sizeof(parts))
    return null;

  // ({ function_name, *synopsis, *params, returns, description, *example })

  // Add the synopsis
  out += "### Synopsis\n\n";
  out += "```c\n";
  out += implode(parts[1], "\n") + "\n";
  out += "```\n";

  // Add the parameters
  if(sizeof(parts[2])) {
    string *param = parts[2];

    out += "\n### Parameters\n\n";
    out += implode(param, "\n") + "\n";
  }

  // Add the returns
  if(sizeof(parts[3])) {
    out += "\n### Returns\n\n";
    out += parts[3] + "\n";
  }

  // Add the description
  out += "\n### Description\n\n";
  out += parts[4] + "\n";

  return out;
}

private nomask string generate_index_markdown(string source_file, mapping funcs) {
  string out = "";
  mixed err;

  err = catch {
    string *function_names, function_name;
    mapping interested;

    interested = filter(funcs, (: $2["source_file"] == $3 :), source_file);
    function_names = sort_array(keys(interested), 1);

    foreach(function_name in function_names) {
      string line;
      string *parts;
      string file;
      mapping func;

      func = interested[function_name];

      parts = dir_file(func["source_file"]);
      file = parts[1];

      // Update link generation to include .md extension
      line = sprintf("* [%s](%s.md#%s)\n",
        function_name,
        chop(file, ".c", -1),
        function_name
      );

      out += sprintf(line);
    }

    return out;
  };

  if(err)
    log_file("system/autodoc", "Error generating markdown: " + err + "\n");

  return null;
}

private nomask void generate_wiki() {
  string *function_types, function_type;

  writing = true;

  function_types = sort_array(keys(docs), 1);

  foreach(function_type in function_types) {
    string *function_names;
    string index_file;
    string current_source_file;
    string dest_dir;
    mapping funcs;
    string *source_files, source_file;
    string index_content = "";

    index_content =
      "---\n"
      "title: " + function_type + "\n"
      "---\n";
    index_content += sprintf("# %s\n\n", function_type);

    funcs = docs[function_type];

    function_names = sort_array(keys(funcs), 1);

    dest_dir = append(wiki_doc_root, function_type + "/");

    source_files = map(values(funcs), (: $1["source_file"] :));
    source_files = distinct_array(source_files);
    source_files = sort_array(source_files, 1);

    index_file = append(wiki_doc_root, function_type + "/index.md");
    if(file_exists(index_file))
      rm(index_file);
    else
      assure_file(index_file);

    foreach(source_file in source_files) {
      string *parts, source_file_name;
      mapping current_funcs;
      string *curr_function_names, curr_function_name;
      string dest_file;
      string index_md;
      string doc_content = "";

      parts = dir_file(source_file);
      source_file_name = chop(parts[1], ".c", -1);

      index_md = generate_index_markdown(source_file, funcs);
      // Update link generation to include .md extension
      index_content += sprintf("## [%s](%s.md)\n\n", source_file_name, source_file_name);
      index_content += index_md + "\n";

      dest_file = sprintf("%s%s/%s.md", wiki_doc_root, function_type, source_file_name);

      current_funcs = filter(funcs, (: $2["source_file"] == $3 :), source_file);
      curr_function_names = keys(current_funcs);

      doc_content = "# " + source_file_name + ".c\n\n";

      foreach(curr_function_name in curr_function_names) {
        mapping func;
        string func_md;

        func = current_funcs[curr_function_name];
        func_md = generate_function_markdown(curr_function_name, func);

        if(func_md) {
          doc_content += sprintf("## %s\n\n", curr_function_name);
          doc_content += func_md;
          doc_content += "\n";
        }
      }

      assure_file(dest_file);
      doc_content =
        "---\n"
        "title: " + source_file_name + "\n"
        "---\n" + doc_content;
      write_file(dest_file, doc_content, 1);
      doc_content = "";
    }

    assure_file(append(wiki_doc_root, function_type+".md"));
    write_file(index_file, index_content, 1);
    index_content = "";
  }

  writing = false;
}

private nomask void finish_scan() {
  float end_time;
  string time_log, file_log, dir_log;

  if(check_running() == true)
    return;

  end_time = time_frac();

  // Generate the mud docs
  generate_mud_docs();

  // Generate the wiki
  generate_wiki();

  end_time = time_frac();

  time_log = sprintf("Scan time: %.2fs", end_time - start_time);
  dir_log = sprintf("Directories scanned: %s", add_commas(total_dirs_scanned));
  file_log = sprintf("Files scanned: %s", add_commas(total_files_scanned));

  _log(1, "Autodoc has completed.");
  _log(1, time_log);
  _log(1, dir_log);
  _log(1, file_log);

  if(this_body() && devp(this_body())) {
    _ok("Autodoc has completed.");
    _ok(time_log);
    _ok(dir_log);
    _ok(file_log);
  }
}

public nomask int check_running() {
  return scanning > 0 || writing == true;
}
