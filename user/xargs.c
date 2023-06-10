#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/param.h"


// int
// main(int argc, char *argv[])
// {
//    char *parm[MAXARG];
//    char order[32];
//    for(int i = 1; i < argc; i++){
//     parm[i - 1] = argv[i];
//    }
//    int cur = argc - 1;
//    int k;
//    while((k = read(0, order, sizeof(order))) > 0){
//         // printf("%s\n", order);
//         int j = 0;
//         for(int i = 0; i < k; i++){
//             if(order[i] == '\n'){
//                 order[i] = 0;
//                 parm[cur++] = &order[j];
//                 parm[cur] = 0;
//                 cur = argc - 1;
//                 j = i + 1;
//                 if(fork() == 0){
//                     exec(argv[1], parm);
//                 }
//                 wait(0);
//             }else if(order[i] == ' '){
//                 order[i] = 0;
//                 parm[cur++] = &order[j];
//                 j = i + 1;
//             }else{
//                 continue;
//             }
//         }
//    }
//    exit(0);
// }

int main(int argc, char *argv[]) {
  int i, count = 0, k, m = 0;
  char* lineSplit[MAXARG], *p;
  char block[32], buf[32];
  p = buf;
  for (i = 1; i < argc; i++) {
    lineSplit[count++] = argv[i];
  }
  while ((k = read(0, block, sizeof(block))) > 0) {
    for (i = 0; i < k; i++) {
      if (block[i] == '\n') {
        buf[m] = 0;
        lineSplit[count++] = p;
        lineSplit[count] = 0;
        m = 0;
        p = buf;
        count = argc - 1;
        if (fork() == 0) {
          exec(argv[1], lineSplit);
        }
        wait(0);
      } else if (block[i] == ' ') {
        buf[m++] = 0;
        lineSplit[count++] = p;
        p = &buf[m];
      } else {
        buf[m++] = block[i];
      }
    }
  }
  exit(0);
}