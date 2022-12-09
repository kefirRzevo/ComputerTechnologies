#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define $$ fprintf(stderr, "%d\n", __LINE__);

int brought = 20;
int* n_portions_ptr;
int* mom_sum_brought_ptr;
useconds_t delay = 10000;

int semid;
union semun arg;

//creating P and V commands for semaphores
struct sembuf P0  = {0, -1,  0};
struct sembuf V0  = {0,  1,  0};
struct sembuf P1  = {1, -1,  0};
struct sembuf V1  = {1,  1,  0};

void take(int n)
{
    (*n_portions_ptr)--;
    fprintf(stderr, "Baby eagle %d took food\n", n);
    usleep(delay);
}

void eat(int n)
{
    fprintf(stderr, "Baby eagle %d eat food\n", n);
    usleep(delay);
}

void sleeep(int n)
{
    fprintf(stderr, "Baby eagle %d sleep\n", n);
    usleep(delay);
}

void call(int n)
{
    if(semop(semid, &V0, 1) == -1)
        return perror("semop:V0 fatal\n");

    fprintf(stderr, "Baby eagle %d called mum\n", n);
    usleep(delay);
}

void bring()
{
    *n_portions_ptr = brought;
    *mom_sum_brought_ptr+=brought;
    fprintf(stderr, "Mother eagle brought %d portions\n", *n_portions_ptr);
    usleep(delay);
}

void mother()
{
    while(*mom_sum_brought_ptr < 100)
    {
        if(semop(semid, &P0, 1) == -1)
            return perror("semop:P0 fatal\n");

        bring();
        if(semop(semid, &V1, 1) == -1)
            return perror("semop:V1 fatal\n");
    }
}

void baby(int n_baby)
{
    while(*mom_sum_brought_ptr < 100)
    {
        if(semop(semid, &P1, 1) == -1)
            return perror("semop:P1 fatal");

        take(n_baby);

        if(*n_portions_ptr == 0)
            call(n_baby);
        else if(semop(semid, &V1, 1) == -1)
            return perror("semop:V1 fatal\n");

        eat(n_baby);
        sleeep(n_baby);
    }
}

int main(int argc, char *argv[])
{
    assert(argc == 2);
    int n_babies = atoi(argv[1]);

    int mem_key = shmget(IPC_PRIVATE, 2*sizeof(int), IPC_CREAT | 0666);
    n_portions_ptr = (int* ) shmat(mem_key, NULL, 0);
    mom_sum_brought_ptr = n_portions_ptr+1;

    //create two semaphores
    semid = semget(228, 2, 0666 | IPC_CREAT);

    /*Initialization*/
    *n_portions_ptr  = brought;
    *mom_sum_brought_ptr = brought;
    //put 0 in the first semaphore
    arg.val = 0;
    semctl(semid, 0, SETVAL, arg);
    //put 1 in the second one
    arg.val = 1;
    semctl(semid, 1, SETVAL, arg);

    //run mother process
    pid_t mother_pid = fork();
    if(mother_pid == 0)
    {
        mother();
        exit(0);
    }

    //run babies processes
    for(size_t i = 0; i < n_babies; i++)
    {
        pid_t baby_pid = fork();
        if(baby_pid == 0)
        {
            baby(i);
            exit(0);
        }
    }

    //wait everyone
    waitpid(mother_pid, NULL, 0);
    for(size_t i = 0; i < n_babies; i++)
        wait(NULL);

    shmdt(n_portions_ptr);
    shmctl(mem_key, IPC_RMID, (struct shmid_ds * )n_portions_ptr);

    printf("The end\n");
    return 0;
}
