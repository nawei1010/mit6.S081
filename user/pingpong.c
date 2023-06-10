#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char* argv[])
{
    int p[2];
    pipe(p);

    if(fork() == 0){
        char revecive_message[100];
        read(p[0], revecive_message, 100);
        printf("%d: received %s\n",getpid(), revecive_message);
        write(p[1], "pong", 4);
        close(p[0]);
        close(p[1]);
        exit(0);
    }else{
        char revecive_message[100];
        write(p[1], "ping", 4);
        read(p[0], revecive_message, 100);
        printf("%d: received %s\n",getpid(), revecive_message);
        close(p[0]);
        close(p[1]);
        exit(0);
    }
}