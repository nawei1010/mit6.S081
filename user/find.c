#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void
find(char* path, char* target)
{
  int fd;
  struct stat st;
  struct dirent de;
  char buf[512], *p;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch (st.type)
  {
  case T_FILE:
    break;
  
  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
        printf("find: path too long\n");
        break;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    // first add a '/'
    *(p++) = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
        if(de.inum == 0){
            continue;
        }
        if(strcmp(".", de.name) == 0 || strcmp("..", de.name) == 0){
            continue;
        }
        strcpy(p, de.name);
        if(strcmp(target, de.name) == 0){
            printf("%s\n", buf);
        }
        stat(p, &st);
        if(st.type == T_DIR){
            find(buf, target);
        }
    }
    break;
  }
  close(fd);
}


int
main(int argc, char *argv[])
{
    if(argc < 3){
        fprintf(2, "find: arguments missed\n");
        exit(1);
    }
    find(argv[1], argv[2]);
    exit(0);
}