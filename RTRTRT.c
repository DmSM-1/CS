#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <error.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#define HELLO   SIGRTMIN
#define SEND    SIGRTMIN + 1
#define GOODBYE SIGRTMIN + 2

int pid;
int ppid;
int mes;
int tts = 0;
union sigval sig;


int tcf(int mes, int tpid){
    for (int i = 0; i < 10; i++){
        usleep(tts);
        tts++;
        if (!sigqueue(tpid, mes, sig)){
            tts--;
            return 0;
        }
    }
    return -1;
}



void writer(int sig, siginfo_t* info, void* ptr){
    if (sig == HELLO){
        tcf(HELLO, ppid);
    }else if(sig == SEND){
        char c = info->si_value.sival_int;
        if (write(1,&c,1) < 0){
            tcf(GOODBYE, ppid);
        }
        else{
            tcf(SEND, ppid);
        }
    }
}


void reader(int mes, siginfo_t* info, void* ptr){
    char c = 0;
    if (mes == GOODBYE || read(0, &c, 1) <= 0){
        kill(pid, SIGKILL);
        exit(EXIT_FAILURE);
    }
    sig.sival_int = c;
    if (mes == HELLO || mes == SEND){
        if (tcf(SEND, pid) < 0){
            kill(pid, SIGKILL);
            exit(EXIT_FAILURE);
        }
    }
}


int init_sig(void* ptr){
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = ptr;
    if (sigaction(HELLO     , &sa, NULL) == -1 ||
        sigaction(SEND      , &sa, NULL) == -1 ||
        sigaction(GOODBYE   , &sa, NULL) == -1)
            return -1;
    return 0;
}


int main(int argc, char* argv[]){
    pid = 0;
    if ((pid = fork()) == -1){
        perror("Fall");
        exit(EXIT_FAILURE);
    }

    if (pid == 0){
        ppid = getppid();
        if (init_sig(&writer) == -1){
            perror("Fall");
            exit(EXIT_FAILURE);
        }  
        while(1){
            pause();
        }
        return 0;
    }

    if (init_sig(&reader) == -1){
        perror("Fall");
        kill(pid, SIGKILL);
        return 0;
    }  

    usleep(10000);
    if (tcf(SEND, pid) < 0){
        kill(pid, SIGKILL);
        exit(EXIT_FAILURE);
    }
    while(1){
        pause();
    }
    kill(pid, SIGKILL);
    return 0;
}