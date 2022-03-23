#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
  int child_pid, pid, st;
  char *data = 0;
  int p1[2], p2[2];

  if (argc != 1)
  {
    fprintf(2, "Usage: pingpong argc = %d != 1...\n", argc);
    exit(1);
  }
  pipe(p1); pipe(p2);

  child_pid = fork();
  pid = getpid();
  
  if (child_pid == 0)
  {
    // child process
    read(p1[0], data, 1);
    fprintf(2, "%d: received ping\n", pid);
    write(p2[1], data, 1);
  }
  else
  {
    // parent process
    write(p1[1], data, 1);
    wait(&st);
    read(p2[0], data, 1);
    fprintf(2, "%d: received pong\n", pid);
  }
  close(p1[0]); close(p1[1]);
  close(p2[0]); close(p2[1]);
  exit(0);
}