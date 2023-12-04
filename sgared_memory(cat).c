#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <error.h>
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>


#define CAPACITY 4096
#define SLEEPTIME 100000
int in = 0;
int out = 1;

#define update(fl, opt)\
    buf.sem_num = fl;\
    buf.sem_op = opt;\
    buf.sem_flg = 0;\
    semop(semid, &buf, 1)

#define flg(fl) printf("[%d]:%d\n",fl, semctl(semid, fl, GETVAL))

#define max(a,b) (a>b)?(a):(b)


void mread(int* count, int semid, int* shmid, struct sembuf buf){
    int w = 0;
    int n = 0;
    void** shared_memory = malloc((*count)*sizeof(void*));
    for(int i = 0; i<(*count);i++)
        shared_memory[i] = shmat(shmid[i], NULL, 0);

    while(1){
        update(w, 0);
        int e = read(in, (char*)(shared_memory[w]+n), CAPACITY - n);

        if (e == -1 && !n){
            usleep(SLEEPTIME);
            continue;
        }
        n += max(e, 0);

        if(n == CAPACITY || (e==-1 && n) || e==0){
            *((int*) (shared_memory[w]+CAPACITY-sizeof(int))) = n;
            update(w, 2);
            w++;
            w %= (*count);
            n = 0;
        }
        if (e == 0){
            break;
        }
    }
    update(*count, 1);
    free(shared_memory);
}


int main(int argc, char** argv){
    int count = 1;
    char* c;
    int num = 0;
    struct sembuf buf = {};
    char charbuf[CAPACITY] = {0};
    if (argc == 2)
        count = strtol(argv[1], &c, 10);
    int semid = semget(IPC_PRIVATE, count+1, 0777|IPC_CREAT);
    if (semid == -1){
        perror("semget:");
        exit(EXIT_FAILURE);
    }
    for(int i = 0; i < count+1; i++){
        if  (semctl(semid, i, SETVAL, 0) == -1){
            perror("semctl");
            exit(EXIT_FAILURE);
        }
    }
    int* shmid = (int*)malloc(count*sizeof(int));
    for(int i = 0; i < count; i++){
        if((shmid[i] = shmget(IPC_PRIVATE, CAPACITY + sizeof(int), 0777|IPC_CREAT))==-1){
            for (int j = 0; j < i; j++)
                close(shmid[j]);
            perror("shmget");
            exit(EXIT_FAILURE);
        }
    }

    const int flags = fcntl(in, F_GETFL, 0);
    fcntl(in, F_SETFL, flags | O_NONBLOCK);
    int w = 0;
    int n = 0;

    int ppid = getpid();
    int cpid = fork();

    if (!cpid){
        void** shared_memory = malloc(count*sizeof(void*));
        for(int i = 0; i<count;i++)
            shared_memory[i] = shmat(shmid[i], NULL, 0);
        

        while(1){
            if (semctl(semid, count, GETVAL) && semctl(semid, w, GETVAL) == 0)
                break;
            update(w, -1);
            int k = *((int*) (shared_memory[w]+CAPACITY-sizeof(int)));
            write(out, (char*)shared_memory[w], k);
            update(w, -1);
            w++;
            w %= count;
        }
        exit(EXIT_SUCCESS);
    }

    if (argc <= 2)
        mread(&count,semid, shmid, buf);
    else if(argc > 2){
        for(int i = 2; i<argc; i++){
            if ((in = open(argv[i], O_RDONLY)) == -1)
                continue;
            mread(&count,semid, shmid, buf);
            close(in);
        }
    }
    

    for(int i = 0; i < count; i++)
        close(shmid[i]);
    free(shmid);
    wait(NULL);
    

    return 0;
}
