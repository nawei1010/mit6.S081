#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void printPrime(int left[2]){
    //接收左边的管道，创建右边的管道
    int right[2];
    int num[11];
    int *cur = num;
    pipe(right);
    close(left[1]);
    while(read(left[0], cur, 4) != 0){
       cur++;
    }
    close(left[0]);
    // means no data
    if(cur == num){
        exit(0);
    }
    printf("prime %d\n", num[0]);
    
    if(fork() == 0){
        printPrime(right);
    }else{
        close(right[0]);
        for(int i = 1; i < cur - num; i++){
            write(right[1], &num[i], 4);
        }
        close(right[1]);
        wait(0);
    }
}

int 
main(int argc, char * argv[])
{   
    int num[11] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31};
    int p[2];
    pipe(p);
    if(fork() == 0){
        printPrime(p);
    }else{
        // 对于初始进程单独进行处理，之后的进程分左右，
        // 第一个进程就一个右，没有左边的部分
        close(p[0]);
        for(int i = 0; i < 11; i++){
            write(p[1], &num[i], 4);
        }
        close(p[1]);
        wait(0);
    }
    exit(0);
}