#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

#define cdump printf("child: buf=%d, bit=%d, pos=%d, fen=%d\n", buf, bit, pos, fen)
#define pdump printf("father: buf=%d, bit=%d, pos=%d, fen=%d\n", buf, bit, pos, fen)


char buf = 0;
char bit = 0;
char pos = 0;
char fen = 0;
int  pid = 0;
int ppid = 0;


void zero(){
    pos++;
    if(pos == 8){
        write(1, &buf, 1);
        pos = 0;
        buf = 0;
    }
    kill(ppid, SIGUSR1);
}

void one(){
    buf += 1<<(pos);
    pos++;
    if(pos == 8){
        write(1, &buf, 1);
        pos = 0;
        buf = 0;
    }
    kill(ppid, SIGUSR1);
}



void read_bit(){
    if (pos == 0){
        fen = read(0, &buf, 1);
        pos = 8;
    }
    bit = buf & 1;
    buf = buf >> 1;
    pos--;
    if (bit){
        kill(pid, SIGUSR2);
    }else{
        kill(pid, SIGUSR1);
    }
}

int main(int argc, char** argv){
    pid = fork();
    if(pid == 0){
        ppid = getppid();
        signal(SIGUSR1, &zero);
        signal(SIGUSR2, &one);
        while(1)
            pause();
        return 0;
    }

    usleep(100000);
    signal(SIGUSR1, &read_bit);
    read_bit();
    while(1){
        if(fen == 0 && pos == 0)
            break;
        pause();
    }

    kill(pid, SIGTERM);

    return 0;
}


