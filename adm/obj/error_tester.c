// /obj/admin/error_finder.c
// This object will crawl the enter game loading all .c files
// to find any that report errors loading.
//
// Do not run this on PRODUCTION!
//
// Created:     2022/03/23: Gesslar
// Last Change: 2022/03/23: Gesslar
//
// 2022/03/23: Gesslar - Created

#include <colour.h>

private nosave float dir_delay = 0.02;
private nosave float file_delay = 0.01;

private nosave string *dirs_to_check = ({ });
private nosave string *files_to_check = ({ });
private nosave string *errs = ({ });
private nosave string *exclude = ({
  ".", "..", "attic", "BACKUP", "old", "backup",
  "ATTIC", ".git", ".vscode", "tmp", "TMP", "u",
  "log", "doc", "plans", "players", "data", "_attic",
  "weapon2", "removed", "cards", "fluffos", ".github", "ARCHIVE",
  "archive", "home", "node_modules",
});

private nosave int file_call_out = 0;
private nosave int dir_call_out = 0;
private nosave int files_checked = 0;
private nosave int dirs_checked = 0;

private nosave float started;
private nosave object notify;

private nosave string FILE_COLOUR = "{{b06ba3}}";
private nosave string DIR_COLOUR  = "{{1fa378}}";

void check_running() {
  if(!sizeof(dirs_to_check) && !sizeof(files_to_check)) {
    message("info", "\aDone!\n", notify);
    message("info", sprintf("%d dirs and %d files checked.\n", dirs_checked, files_checked), notify);
    message("info", sprintf("Duration: %.2fs\n", time_frac()-started), notify);
  }
}

void runit(object who) {
  notify = who;

  started = time_frac();
  message("info", "=== STARTING THE ERROR FINDER ===\n", notify);

  dirs_to_check = ({ "/" });

  dir_call_out = call_out_walltime("check_dir", dir_delay);

  rm("/log/ERRORS");
}

void check_dir() {
  string *paths, *files, *dirs;
  string current;

  check_running();

  if(sizeof(dirs_to_check) > 0)
    dir_call_out = call_out_walltime("check_dir", dir_delay);
  else
    return;

  dirs_checked++;

  current = dirs_to_check[0];

  message("info", DIR_COLOUR + "CHECKING: " + current + "{{res}}\n", notify);

  current = append(current, "/");
  dirs_to_check = dirs_to_check[1..];

  paths = get_dir(current);
  paths -= exclude;
  paths = map(paths, (: $2 + $1 :), current);
  files = filter(paths, (: file_size($1) > 0 && $1[<2..] == ".c" :));
  dirs = filter(paths, (: file_size($1) == -2 :));

  dirs_to_check += dirs;
  files_to_check += files;

  if((file_call_out == 0 || find_call_out(file_call_out) == -1) && sizeof(files_to_check) > 0)
    file_call_out = call_out_walltime("check_file", file_delay);
}

void check_file() {
  string file;
  mixed err;

  check_running();

  if(!sizeof(files_to_check))
    return;

  files_checked++;

  file_call_out = call_out_walltime("check_file", file_delay);

  file = files_to_check[0];

  message("info", FILE_COLOUR + " - CHECKING: " + file + "{{res}}\n", notify);

  files_to_check = files_to_check[1..];

  if(err = catch(test_load(file))) {
    errs += ({ ({ file, err }) });
    write_file("/log/ERRORS", file + "\n" + err + "\n");
  }
}
