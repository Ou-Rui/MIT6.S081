#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

char **read_extra(char **xargv, int len);

int main(int argc, char *argv[]) 
{
  char *xargv[MAXARG] = {0};
  int fd = 0;
  char buf[512];
  int buf_index = 0, buf_start = 0;
  int index = 0;

  for (int i = 1; i < argc; i++)
    xargv[index++] = argv[i];
  
  char **xargv1 = read_extra(xargv, argc);
  // printf("xargv = \n");

  while(1) {
    int read_len = 0;
    while (1)
    {
      read_len = read(fd, &buf[buf_index], 1);
      if (read_len == 0 || buf[buf_index] == '\n')
        break;
      buf_index++;
    }
    if (read_len == 0)
      break;
    buf[buf_index] = 0;
    xargv[index++] = &buf[buf_start];

    buf_start = ++buf_index;
  }
  

  if (fork() == 0) {
    exec(argv[1], xargv1);
  }
  else
  {
    wait(0);
  }
  exit(0);
}


char **read_extra(char **xargv, int index)
{


  return xargv;
}
