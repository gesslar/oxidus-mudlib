//shutdown_d.c

//Tactius @ LPUniversity
//22-OCT-05
//Shutdown daemon

//Nothing fancy, but it works :) -- Tacitus

//Last edited on October 23rd, 2005 by Tacitus

#include <logs.h>
#include <commands.h>
#include <shutdown.h>

inherit STD_DAEMON;

int start(int minutes, int type);
private nomask void do_shutdown();
int stop();
void check();
string get_status();

int start_time;
int end_time;
int type;
int running = 0;

int start(int minutes, int type_in) {
  object *users = users();
  string mess;

  type = type_in;
  if(!caller_is(CMD_SHUTDOWN) && !caller_is(CMD_REBOOT))
    return 0;

  if(find_call_out(running) > -1)
    return 0;

  mess =
    sprintf("%s will be %s %s.",
      mud_name(),
      type == SYS_SHUTDOWN ? "shutting down" : "rebooting",
      minutes ? "in " + minutes +" minutes" : "now"
    );

  filter(users, (: _warn($1, $(mess)) :));

  if(minutes == 0) {
    do_shutdown();
  } else {
    start_time = time();
    end_time = time() + (60 * minutes);
    running = call_out("do_shutdown", minutes * 60);

    if(type == SYS_SHUTDOWN)
      emit(SIG_SYS_SHUTTING_DOWN, end_time-start_time);
    else
      emit(SIG_SYS_REBOOTING, end_time-start_time);

    call_out("check", 1);
  }

  return 1;
}

int stop() {
  string mess;

  if(find_call_out(running) == -1)
      return 0;

  if(!caller_is(CMD_SHUTDOWN) && !caller_is(CMD_REBOOT))
    return 0;

  if(type == SYS_SHUTDOWN)
    mess = "Shutdown Canceled.";
  else
    mess = "Reboot Canceled.";

  filter(users(), (: _warn($1, $(mess)) :));

  remove_call_out();
  running = 0;

  if(type == SYS_SHUTDOWN)
    emit(SIG_SYS_SHUTDOWN_CANCEL);
  else
    emit(SIG_SYS_REBOOT_CANCEL);

  return 1;
}

private nomask void do_shutdown() {
  object *all = users();

  filter(all, (: $1->save_body() :));
  log_file(LOG_LOGIN, sprintf(
    "Driver %s on %s. All users forced to quit.\n",
    type == SYS_SHUTDOWN ? "shut down" : "rebooted",
    ctime())
  );

  filter(all, (: _warn($1, "The mud is shutting down now.") :));
  log_file(LOG_SHUTDOWN, sprintf(ctime(time()) + ": " + mud_name() + "%s", type == SYS_SHUTDOWN ? " shutting down.\n" : " rebooting.\n"));

  shutdown(type);
}

void check() {
  int time_left;
  object *all = users();

  if((time_left = find_call_out(running)) == -1)
    return;

  if((time_left % 300) == 0 && time_left < 1200 || time_left == 30 || time_left == 10) {
    string message = sprintf("%s will be %s %s %s.",
      mud_name(),
      type == SYS_SHUTDOWN ? "shutting down" : "rebooting",
      "in " + (time_left / 60)  + " minutes and",
      (time_left % 60) + " seconds"
    );

    filter(all, (: _warn($1, $(message)) :));
  }

  if(type == SYS_SHUTDOWN)
    emit(SIG_SYS_SHUTTING_DOWN, time_left);
  else
    emit(SIG_SYS_REBOOTING, time_left);

  call_out("check", 1);
}

string get_status() {
  if(find_call_out(running) == -1)
    return null;

  return sprintf("%s will be %s in %d minutes and %d seconds.",
      mud_name(),
      type == SYS_SHUTDOWN ? "shutting down" : "rebooting",
      (end_time - time()) / 60,
      (end_time - time()) % 60
    );
}
