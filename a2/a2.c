
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "a2_helper.h"

typedef struct {
    int proc;
    int val;
    int max;
    sem_t *logSemN;
    pthread_mutex_t *lock;
    pthread_cond_t *cond;
}Th_structure;

pid_t p1,p2, p3, p4, p5, p6, p7;
int x = 0;


void* threadFn5(void * p){
    Th_structure param = *(Th_structure*) p;

    sem_wait(param.logSemN);
        info(BEGIN, param.proc, param.val);

        pthread_mutex_lock(param.lock);
            pthread_cond_signal(param.cond);
            x++;
            pthread_cond_signal(param.cond);
        pthread_mutex_unlock(param.lock);
        usleep(param.val * 900);
        pthread_mutex_lock(param.lock);
            printf("\n                                %d threaduri\n", x);
            if(param.val == 11){
                while(x != 5) {
                    pthread_cond_wait(param.cond, param.lock);
                }
                info(END, param.proc, param.val);
                pthread_mutex_unlock(param.lock);
                sem_post(param.logSemN);
                x--;
                return NULL;
            }           
        pthread_mutex_unlock(param.lock);

        usleep(param.val * 900);

        pthread_mutex_lock(param.lock);
            pthread_cond_signal(param.cond);
            x--;
            pthread_cond_signal(param.cond);
        pthread_mutex_unlock(param.lock);

        info(END, param.proc, param.val);
        //x--;
    sem_post(param.logSemN);
    return NULL;
}

void* threadFn6(void * p){
    Th_structure param = *(Th_structure*)p;
    info(BEGIN, param.proc, param.val);
    Th_structure param5;
    pthread_t tid;
    param5.val = 5;
    param5.proc = param.proc;
    if(param.val == 3){
        if(pthread_create(&tid, NULL, threadFn6, &param5) != 0){
            perror("Error creating thread");
            return NULL;
        }
        pthread_join(tid, NULL);
    }
    info(END, param.proc, param.val);
    return NULL;
}

int main(){
    init();  
    info(BEGIN, 1, 0);

    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t lock;
    if(pthread_mutex_init(&lock, NULL) != 0) {
        perror("error initializing the mutex");
        return 1;
    }

    p2 = fork();
    
    if(p2 == -1){
        printf("Error p2");
        return -1;
    } else if(p2 == 0){
        info(BEGIN, 2, 0);

        p5 = fork();
        if(p5 == -1){
            printf("Error p5");
            return -1;
        } else if( p5 == 0){
            info(BEGIN, 5, 0);
            sem_t logSemN;
            if(sem_init(&logSemN, 0, 5) != 0 ) {
                perror("Could not init the semaphore");
                return -1;
            }

            Th_structure param[46]; //, auxP;
            pthread_t thrId[46];
            // pthread_t aux;
            // auxP.cond = &cond;
            // auxP.lock = &lock;
            // if(pthread_create(&aux, NULL, threadFnAux, &auxP) != 0){
            //         perror("Error creating thread");
            //         return -1;
            // }

            for(int i = 0; i < 46; i++){
                param[i].cond = &cond;
                param[i].lock = &lock;
                param[i].logSemN = &logSemN;
                param[i].val = i+1;
                param[i].proc = 5;
                param[i].max = 5;
                if(pthread_create(&thrId[i], NULL, threadFn5, &param[i]) != 0){
                    perror("Error creating thread");
                    return -1;
                }
            }
            for(int i = 0; i<46; i++){
                pthread_join(thrId[i], NULL);
            }

            info(END, 5, 0);
        }else {
            p6 = fork();
            if(p6 == -1){
                printf("Error p6");
                return -1;
            }else if( p6 == 0){
                info(BEGIN, 6, 0);
                    pthread_t thrId[4];
                    Th_structure param[4];
                    for(int i = 0; i < 4; i++){
                        param[i].val = i+1;
                        param[i].proc = 6;
                        if(pthread_create(&thrId[i], NULL, threadFn6, &param[i]) != 0){
                            perror("Error creating thread");
                            return -1;
                        }
                    }
                    for(int i = 0; i<4; i++){
                        pthread_join(thrId[i], NULL);
                    }
                info(END, 6, 0);
            }else {
                p7 = fork();
                if(p7 == -1){
                    printf("Error p7");
                    return -1;
                }else if( p7 == 0){
                    info(BEGIN, 7, 0);
                    info(END, 7, 0);
                } else {
                    wait(NULL); wait(NULL); wait(NULL); //le asteapta pe 5 6 7
                    info(END, 2, 0);
                }
            }
        }
        
    }
    else {

        p3 = fork();
        if(p3 == -1){
            printf("Error p3");
            return -1;
        } else if(p3 == 0){
            info(BEGIN, 3, 0);

            p4 = fork();
            if(p4 == -1){
                printf("Error p4");
                return -1;
            }else if( p4 == 0){
                info(BEGIN, 4, 0);
                info(END, 4, 0);
            } else {
                wait(NULL); // il asteapta pe 4
                info(END, 3, 0);
            }

        } else {
            wait(NULL); //il asteapta pe 2
            wait(NULL); //il asteapta pe 3
            
            info(END, 1, 0);
        }

    }
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&lock);
    return 0;
}