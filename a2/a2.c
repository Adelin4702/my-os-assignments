
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "a2_helper.h"

sem_t *sem1, *sem4, *semN, *sem22, *sem21, *sem41, *sem42;

typedef struct {
    int proc;
    int val;
    pthread_cond_t *cond;
    pthread_mutex_t *lock;
}Th_structure;

pid_t p1,p2, p3, p4, p5, p6, p7;
int x = 0, nrThreads = 0, ok = 0;


void* threadFn5(void * p){
    Th_structure param = *(Th_structure*) p;

    sem_wait(semN);

        info(BEGIN, param.proc, param.val);

        pthread_mutex_lock(param.lock);   //numaram threadurile care au intrat, in total
            nrThreads++;
        pthread_mutex_unlock(param.lock);
    
        if( param.val == 11){    //daca am ajuns la thr 11, marcam prin var ok
            pthread_mutex_lock(param.lock);
            ok = nrThreads;
            pthread_mutex_unlock(param.lock);  // daca nu asteapta destule threaduri dupa thr 11 sau daca nu e ultimul thread, atunci asteapta
            if(x < 4 || ok != 46){
                sem_wait(sem1);
            }
        }
        pthread_mutex_unlock(param.lock);

        pthread_mutex_lock(param.lock);
        if(x < 4 && param.val != 11 && (nrThreads > 41 || ok > 0)){  /// daca nu avem deja 4 thr care asteapta, thr curent nu e 11 si (suntem la ultimele threaduri sau thr 11 asteapta)
            x++;          /// crestem nr de thr care asteapta
            if(/*ok < 43 &&*/ x == 4){  // daca am ajuns la 4 thr care asteapta,  pornim thr 11; ca sa nu punem thr in asteptare daca 11 asteapta deja
                sem_post(sem1);
            }
            pthread_mutex_unlock(param.lock);
            pthread_cond_wait(param.cond, param.lock); // asteptam semnalul ca thr11 a terminat   ---
        }
        pthread_mutex_unlock(param.lock);

        if(x == 4){  // verificam ca thr asteapta pt situatia in care, deja asteapta 4 threaduri dupa thr 11
            sem_post(sem1);
        }

        if(param.val == 11){  // daca am ajuns cu thr 11 la final, pornim si celelalte thr
            info(END, param.proc, param.val);
            pthread_cond_broadcast(param.cond);
        } else {
            info(END, param.proc, param.val);
            sem_post(semN);
        }
    return NULL;
}

void* threadFn6(void * p){
    Th_structure param = *(Th_structure*)p;

    if(param.val == 5 && param.proc == 6){  //asteptam sa primim permisiune ca T6.3 a inceput
        sem_wait(sem21);
    }

    if(param.val == 4 && param.proc == 6){  //asteptam sa primim permisiune ca T2.4 a terminat
        sem_wait(sem41);
    }

    if(param.val == 2 && param.proc == 2){  //asteptam sa primim permisiune ca T6.4 a terminat
        sem_wait(sem42);
    }

    info(BEGIN, param.proc, param.val);

    if(param.val == 3 && param.proc == 6){  
        sem_post(sem21);  // dam permisiune ca T6.3 a inceput
        sem_wait(sem22);  // asteptam sa primim permisiune ca T6.5 a terminat
    }

    info(END, param.proc, param.val);

    if(param.val == 5 && param.proc == 6){ // dam permisiune ca T6.5 a terminat
        sem_post(sem22);
    }

    if(param.val == 4 && param.proc == 2){ // dam permisiune ca T2.4 a terminat
        sem_post(sem41);
    }

    if(param.val == 4 && param.proc == 6){ // dam permisiune ca T6.4 a terminat
        sem_post(sem42);
    }
    return NULL;
}

int main(){
    init();  
    info(BEGIN, 1, 0);

    sem_unlink("sem42");
    sem_unlink("sem41");
    sem_unlink("sem22");
    sem_unlink("sem21");
    sem_unlink("semN");
    sem_unlink("sem4");
    sem_unlink("sem1");

    sem41 = sem_open("sem41", O_CREAT, 0644, 0);
    sem42 = sem_open("sem42", O_CREAT, 0644, 0);
    sem22 = sem_open("sem22", O_CREAT, 0644, 0);
    sem21 = sem_open("sem21", O_CREAT, 0644, 0);
    sem1 = sem_open("sem1", O_CREAT, 0644, 0);
    sem4 = sem_open("sem4", O_CREAT, 0644, 0);
    semN = sem_open("semN", O_CREAT, 0644, 5);

    p2 = fork();
    
    if(p2 == -1){
        printf("Error p2");
        return -1;
    } else if(p2 == 0){
        info(BEGIN, 2, 0);

        Th_structure param[46]; 
        pthread_t thrId[46]; 

        for(int i = 0; i < 6; i++){
            param[i].val = i + 1;
            param[i].proc = 2;
            if(pthread_create(&thrId[i], NULL, threadFn6, &param[i]) != 0){
                perror("Error creating thread");
                return -1;
            }
        }       

        p5 = fork();
        if(p5 == -1){
            printf("Error p5");
            return -1;
        } else if( p5 == 0){
            info(BEGIN, 5, 0);

            pthread_mutex_t lock;
            pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

            if(pthread_mutex_init(&lock, NULL) != 0) {
                perror("error initializing the mutex");
                return 1;
            }

            Th_structure param[46]; 
            pthread_t thrId[46];

            for(int i = 0; i < 46; i++){
                param[i].lock = &lock;
                param[i].val = i+1;
                param[i].proc = 5;
                param[i].cond = &cond;
                if(pthread_create(&thrId[i], NULL, threadFn5, &param[i]) != 0){
                    perror("Error creating thread");
                    return -1;
                }
            }

            for(int i = 0; i<46; i++){
                pthread_join(thrId[i], NULL);
            }
            pthread_mutex_destroy(&lock);
            pthread_cond_destroy(&cond);
            info(END, 5, 0);
        }else {
            p6 = fork();
            if(p6 == -1){
                printf("Error p6");
                return -1;
            }else if( p6 == 0){
                info(BEGIN, 6, 0);                    
                    pthread_t thrId[5];
                    Th_structure param[5];
                    for(int i = 4; i >= 0; i--){
                        param[i].val = i+1;
                        param[i].proc = 6;
                        if(pthread_create(&thrId[i], NULL, threadFn6, &param[i]) != 0){
                            perror("Error creating thread");
                            return -1;
                        }

                    }
                    for(int i = 4; i >= 0; i--){
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
    return 0;
}