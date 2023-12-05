#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <error.h>

#define update(id, sem, opt)\
    buf.sem_num = sem;\
    buf.sem_op = opt;\
    buf.sem_flg = 0;\
    semop(id, &buf, 1)

#define flg(semid, sem) printf("[%d]:%d\n",sem, semctl(semid, sem, GETVAL));

#define min(a,b) (a<b)?(a):(b)

#define control_zone    0
#define land_ship       1
#define trap            2
#define ship_land       3
#define ship            4
#define trip            5
#define tickets         6
#define bilet           7

#define SEM_COUNT       8
#define TRIP_COUNT      5
#define PASSANGER_COUNT 80
#define SHIP_CAPACITY   25
#define TRAP_CAPACITY   4
#define TICKETS_COUNT   79


void passanger(int* semid, int id){
    struct sembuf buf = {};
    int pid = getpid();

    printf("Пасажир [%d,pid:%d] в порту\n", id, pid);
    
    update(*semid, bilet, -1);
    printf("Пасажир [%d,pid:%d] подошёл к кассе\n", id, pid);
    if(!semctl(*semid, tickets, GETVAL)){
           printf("Пасажир [%d,pid:%d] плачет;(\n", id, pid);
           update(*semid, bilet, 1);
           exit(EXIT_SUCCESS);
    }
    update(*semid, tickets, -1);
    update(*semid, bilet, 1);
    printf("Пасажир [%d,pid:%d] купил билет\n", id, pid);

    update(*semid, control_zone, -1);
    printf("Пасажир [%d,pid:%d] прошёл регистрацию\n", id, pid);

    update(*semid, land_ship, 0);
    update(*semid, trap, -1);
    printf("Пасажир [%d,pid:%d] зашёл на трап\n", id, pid);
    update(*semid, trap, 1);
    update(*semid, ship, -1);
    printf("Пасажир [%d,pid:%d] зашёл на корабль\n", id, pid);

    update(*semid, ship_land, 0);
    update(*semid, trap, -1);
    printf("Пасажир [%d,pid:%d] сошёл на трап\n", id, pid);
    update(*semid, trap, 1);
    update(*semid, ship, 1);    
}

void Mega_ship(int* semid){
    struct sembuf buf = {};
    int pid = getpid();

    semctl(*semid, ship, SETVAL, SHIP_CAPACITY);
    printf("Лайнер в порту [pid:%d]\n", pid);

    semctl(*semid, trip, SETVAL, TRIP_COUNT);
    printf("Маршрут спланирован\n");

    semctl(*semid, tickets, SETVAL, TICKETS_COUNT);
    semctl(*semid, bilet, SETVAL, 1);
    printf("%d билетов поступили в продажу\n", TICKETS_COUNT);

    int not_full = 0;
    int end_cop = 0;
    int trip_count = TRIP_COUNT;
    int mv = min(PASSANGER_COUNT, TICKETS_COUNT);
    if(TRIP_COUNT * SHIP_CAPACITY != mv){
        end_cop = mv%SHIP_CAPACITY;
        semctl(*semid, trip, SETVAL, mv/SHIP_CAPACITY + (end_cop>0));
        trip_count = mv/SHIP_CAPACITY + (end_cop>0);
        not_full = 1;
    }

    for(int i = 0; i < trip_count; i++){
        update(*semid, trip, -1);
        printf("Подготовка трапа\n");
        semctl(*semid, trap, SETVAL, TRAP_CAPACITY);
        semctl(*semid, land_ship, SETVAL, 0);
        semctl(*semid, ship_land, SETVAL, 1);
        printf("Трап готов для входа\n");

        if(!semctl(*semid, trip, GETVAL) && not_full){
            semctl(*semid, control_zone, SETVAL, end_cop);
            semctl(*semid, ship, SETVAL, end_cop);
        }
        else
            semctl(*semid, control_zone, SETVAL, SHIP_CAPACITY);
        printf("Начало регистрации\n");

        update(*semid, control_zone, 0);
        printf("Конец регистрации\n");

        update(*semid, ship, 0);
        update(*semid, trap, -TRAP_CAPACITY);
        semctl(*semid, land_ship, SETVAL, 0);
        printf("Трап поднят\n");
        printf("Лайнер отплывает [pid:%d]\n", pid);
        
        usleep(350000);
        printf("Лайнер приплывает [pid:%d]\n", pid);

        printf("Подготовка трапа\n");
        semctl(*semid, trap, SETVAL, TRAP_CAPACITY);
        semctl(*semid, land_ship, SETVAL, 1);
        semctl(*semid, ship_land, SETVAL, 0);
        printf("Трап готов для выхода\n");

        if(!semctl(*semid, trip, GETVAL) && not_full){
            update(*semid, ship, -end_cop);
            semctl(*semid, trap, SETVAL, -TRAP_CAPACITY);
            break;
        }
        else{
            update(*semid, ship, -SHIP_CAPACITY);
            update(*semid, ship, SHIP_CAPACITY);
        }
    }
}


int main(int argc, char** argv){
    int semid = 0;
    if((semid = semget(IPC_PRIVATE, SEM_COUNT, 0777|IPC_CREAT)) == -1){
        perror("semget");
        exit(EXIT_FAILURE);
    }
    for(int i = 0; i < SEM_COUNT; i++)
        semctl(semid, i, SETVAL, 0);
    
    int pasid[PASSANGER_COUNT];
    for(int i = 0; i < PASSANGER_COUNT; i++){
        if((pasid[i] = fork()) == -1){
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if(!pasid[i]){
            passanger(&semid, i);
            printf("Пасажир [%d,pid:%d] сошел с корабля\n", i, getpid());
            exit(EXIT_SUCCESS);
        }
    }
    int shipid = 0;
    if((shipid = fork()) == -1){
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if(!shipid){
        Mega_ship(&semid);
        printf("Корабль отправляется на обслуживание\n");
        exit(EXIT_SUCCESS);
    }

    for (int i = 0; i < PASSANGER_COUNT + 1; i++){
        wait(NULL);
    }
    return 0;
}