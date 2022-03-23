#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

const int MAX_PRIME = 34;

int child(int p1)
{
  int prime;
  int p2[2];
  // int pid = getpid();
  int data = -1;
  // read first number as prime
  read(p1, (char *)&prime, 4);
  // printf("%d: receive %d\n", pid, prime);
  if (prime == 0)
  {
    close(p1);
    return 0;
  }
  printf("prime %d\n", prime);
  

  pipe(p2);

  if (fork() == 0)
  {
    // grand child
    close(p2[1]);   // child don't need to write
    child(p2[0]);
  }
  else
  {
    // parent
    close(p2[0]);   // parent don't need to read
    while (read(p1, (char *)&data, 4) != 0)
    {
      if (data % prime != 0)
      {
        write(p2[1], (char *)&data, 4);
      }
    }
    close(p2[1]);
    wait(0);
  }
  close(p1);
  return 0;
}


int main(int argc, char *argv[])
{
  if (argc != 1)
  {
    fprintf(2, "Usage: primes argc = %d != 1...\n", argc);
    exit(1);
  }
  int p[2];
  pipe(p);

  if (fork() == 0)
  {
    // child process
    close(p[1]);
    child(p[0]);
  }
  else
  {
    // root, write all numbers into pipe
    for (int i = 2; i < MAX_PRIME; i++)
    {
      write(p[1], (char *)(&i), 4);
    }
    close(p[0]); close(p[1]);
    wait(0);
  }

  exit(0);
}