#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <error.h>


typedef struct Wall{
    pthread_mutex_t mutex;
    pthread_cond_t f_cond, c_cond;
    int f, c, f_wait, c_wait, n, h;
}wall;


void init_wall(wall *w, int *width, int *height){
    pthread_mutex_init(&w->mutex, NULL);
    pthread_cond_init(&w->f_cond, NULL);
    pthread_cond_init(&w->c_cond, NULL);
    w->f = 0;
    w->c = *width;
    w->f_wait = 0;
    w->c_wait = 0;
    w->n = *width;
    w->h = *height;
}


void dest_wall(wall *w){
    pthread_mutex_destroy(&w->mutex);
    pthread_cond_destroy(&w->f_cond);
    pthread_cond_destroy(&w->c_cond);
}


void* father(void* ptr){
    wall *w = (wall*)ptr;
    int goal = (w->h/2+w->h%2)*w->n;
    printf("Father is ready to start\n");
    while(w->f < goal){
        pthread_mutex_lock(&w->mutex);
        if(w->f >= w->c){
            w->f_wait = w->f+1;
            printf("Father waits [%d] %d\n", w->f_wait,w->c);
            pthread_cond_wait(&w->f_cond, &w->mutex);
        }
        printf("Father put %d briks\n", (w->f/w->n)*2*w->n+w->f%w->n+1);
        w->f++;
        pthread_mutex_unlock(&w->mutex);
        
        if(w->c_wait == w->f)
            pthread_cond_signal(&w->c_cond);
        if(w->f== goal)
            break;
                
    }
    printf("Father is relaxing\n");
    return NULL;
}


void* son(void* ptr){
    wall *w = (wall*)ptr;
    int goal = (w->h/2+1)*w->n;
    printf("Son is ready to start\n");
    while(w->c < goal){
        pthread_mutex_lock(&w->mutex);
        
        if(w->c - w->f >= w->n){
            w->c_wait = w->c-w->n+1;
            printf("Son waits [%d] %d\n", w->c_wait,w->f);
            pthread_cond_wait(&w->c_cond, &w->mutex);
        }
        printf("Son put %d briks\n", ((w->c-w->n)/w->n)*2*w->n+w->c%w->n+1+w->n);
        w->c++;
        pthread_mutex_unlock(&w->mutex);
        if(w->f_wait == w->c)
            pthread_cond_signal(&w->f_cond);
        if(w->c == goal)
            break;

    }
    printf("Son is relaxing\n");
    return NULL;
}


int main(int argc, char** argv){
    int width = 0;
    int hight = 0;
    char *c;

    if (argc != 3){
        perror("argc");
        exit(EXIT_FAILURE);
    }
    if ((hight = strtol(argv[1], &c, 10)) == 0 || (width = strtol(argv[2], &c, 10)) == 0){
        perror("strtol");
        exit(EXIT_FAILURE);
    }
    wall great_wall;
    init_wall(&great_wall, &width, &hight);

    pthread_t father_id;
    pthread_t son_id;
    if (pthread_create(&father_id, NULL, father, &great_wall) == -1){
        perror("pthread create");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&son_id, NULL, son, &great_wall) == -1){
        perror("pthread create");
        exit(EXIT_FAILURE);
    }

    pthread_join(father_id, NULL);
    pthread_join(son_id, NULL);

    dest_wall(&great_wall);
    return 0;
}
