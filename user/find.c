#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void find(char *path, char *target);
char* fmtname(char *path);


int main(int argc, char *argv[]) 
{
  if (argc != 3)
  {
    printf("find: argc = %d != 3...\n", argc);
    exit(1);
  }
  find(argv[1], argv[2]);
  exit(0);
}

void find(char *path, char *target)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  // printf("find: path = %s, format = %s，target = %s\n", path, fmtname(path), target);

  if((fd = open(path, 0)) < 0){
    printf("find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    printf("find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    if(strcmp(fmtname(path), target) == 0)
    {
      printf("%s\n", path);
    }
    
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
      if(strcmp(fmtname(buf), target) == 0)
      {
        printf("%s\n", buf);
      }
      // neither . nor .. && TYPE = directory, recursion 
      if ((strcmp(fmtname(buf), ".") != 0 && strcmp(fmtname(buf), "..") != 0) && st.type == T_DIR)
      {
        find(buf, target);
      }
    }
    break;
  }
  close(fd);
  
}

char* fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), 0, DIRSIZ-strlen(p));
  return buf;
}