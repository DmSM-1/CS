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

#define TIMES 100000

#define update(fl, opt)\
    buf.sem_num = fl;\
    buf.sem_op = opt;\
    buf.sem_flg = 0;\
    semop(semid, &buf, 1)

#define flg(fl) printf("[%d]:%d\n",fl, semctl(semid, fl, GETVAL))

#define min(a,b) (a<b)?(a):(b)

typedef struct Prod{
    int num;
    int value;
}prod;


int worker(int id, int semid, int endid, int shmid){
    struct sembuf buf = {};
    prod* shared_memory = shmat(shmid, NULL, 0);
    while(1){
        update(id, -1);
        if(semctl(endid, 0, GETVAL))
            return 0;
        if(shared_memory[id].num != -1){
            printf("Worker[%2d]: %3d[%3d] -> ", id, shared_memory[id].num, shared_memory[id].value);
            shared_memory[id].value += id+1;
            printf("%3d[%3d] (work)\n", shared_memory[id].num, shared_memory[id].value);
        }
        usleep(TIMES);
        update(id, -1);
    }
}


int main(int argc, char** argv){
    if (argc < 3){
        perror("argc");
        exit(EXIT_FAILURE);
    }
    char *c;
    int N = strtol(argv[1], &c, 10);
    int M = strtol(argv[2], &c, 10);

    int semid = semget(IPC_PRIVATE, N, 0777|IPC_CREAT);
    if (semid == -1){
        perror("semget");
        exit(EXIT_FAILURE);
    }
    for(int i = 0; i < N; i++)
        semctl(semid, i, SETVAL, 0);

    int endid = semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
    if (endid == -1){
        perror("semget");
        exit(EXIT_FAILURE);
    }
    semctl(endid, 0, SETVAL, 0);


    int shmid = shmget(IPC_PRIVATE, N*sizeof(prod), 0777|IPC_CREAT);
    if (shmid == -1){
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    for(int i = 0; i<N; i++){
        int pid = fork();
        if (!pid){
            worker(i, semid, endid, shmid);
            exit(EXIT_SUCCESS);
        }
    }

    struct sembuf buf = {};
    prod* shared_memory = shmat(shmid, NULL, 0);
    for(int j = 0; j<N; j++){
        shared_memory[j].num = -1;
        shared_memory[j].value = 0;
    }

    for(int j = 0; j<M+N+1; j++){
        
        for(int i = 0; i<N; i++)
            update(i, 0);
        if (j == M+N)
            semctl(endid, 0, SETVAL, 1);

        printf("Zero starts flags\n");

        if(shared_memory[N-1].num != -1)
            printf("Выход конвеера: изделие %d [%d]\n", shared_memory[N-1].num, shared_memory[N-1].value);
        else if(j < N+M)
            printf("Готовые изделия будут позже\n");
        else
            printf("Конвеер остановлен\n");

        for(int i = N-1; i > 0; i--){
            shared_memory[i].num = shared_memory[i-1].num;
            shared_memory[i].value = shared_memory[i-1].value;
        }

        if(j<M){
            shared_memory[0].num = j;
            shared_memory[0].value = 1;
        }else{
            shared_memory[0].num = -1;
            shared_memory[0].value = 0;
        }
        if (j < M+N){
            printf("Конвеер:");
            for(int i = 0; i<N; i++)
                printf("%d:[%d] ", shared_memory[i].num, shared_memory[i].value);
            printf("\n");
        }

        for(int i = 0; i<N; i++){
            semctl(semid, i, SETVAL, 2);
        }

    }

    for(int i = 0; i<N; i++)
        wait(NULL);

    return 0;
}