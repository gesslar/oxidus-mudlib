// /adm/daemons/gh.c
//
// Created:     2022/03/17: Gesslar
// Last Change: 2022/03/17: Gesslar
//
// 2022/03/17: Gesslar - Created

inherit STD_DAEMON;

private nosave mapping fds = ([ ]);

#define GH_CMD 1

void read_call_back(int fd, mixed mess);
void write_call_back(int fd);
void close_call_back(int fd);

private nosave string *base_args = ({ "/" });

void fetch() {
  int fd;
  mixed *to_send;

  to_send = base_args;

  fd = external_start(GH_CMD,
    to_send,
    (: read_call_back :),
    (: write_call_back :),
    (: close_call_back :)
  );

  if(fd < 0)
    return;

  fds[fd] = ([
    "data" : "",
    "start_time" : perf_counter_ns(),
  ]);
}

void read_call_back(int fd, string mess)
{
    fds[fd]["data"] += mess;
}

void write_call_back(int fd)
{
    // n/a
}

void close_call_back(int fd)
{
  debug("Data %O", fds[fd]["data"]);
  map_delete(fds, fd);
}
