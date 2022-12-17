#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>

//#define __DEBUG__

#ifdef __DEBUG__
#define $$ fprintf(stderr, "%d\n", __LINE__);
#else
#define $$
#endif

#define dump(...) do{ fprintf(stderr, __VA_ARGS__); } while(0)

int semid = 0;
const useconds_t delay = 1000;

enum sems
{
    trap = 0,
    sem_access = 1,
    sem_exit = 2,
    sem_kill = 3,
};


int is_error(const char* msg)
{
    fprintf(stderr, "%s", msg);
    return -1;
}

int boat(int n_trips, int boat_capacity, int n_passangers)
{
    struct sembuf V_access = {sem_access, 1, 0};
    struct sembuf W_access = {sem_access, 0, 0};
    struct sembuf V_exit = {sem_exit, 1, 0};
    struct sembuf W_exit = {sem_exit, 0, 0};
    struct sembuf W_num2_trap[2] = {{trap, -2, 0}, {trap, 2, 0}};
    
    for(size_t i = 0; i < n_trips; i++)
    {
        dump("Новый заход\n");

        for(int n_pas = 0; n_pas < boat_capacity; n_pas++)
        {
            if(semop(semid, &V_access, 1) == -1)
                return is_error("Bad semop V_access\n");
            if(semop(semid, &W_access, 1) == -1)
                return is_error("Bad semop W_access\n");
        }

        if(semop(semid, W_num2_trap, 2) == -1)   
            return is_error("Bad semop W_num2_access\n");

        dump("Корабль уплыл\n");
        usleep(delay);
        dump("Корабль приплыл\n");

        for(int n_pas = boat_capacity; n_pas > 0; n_pas--)
        {
            if(semop(semid, &V_exit, 1) == -1)
                return is_error("Bad semop V_exit\n");
            if(semop(semid, &W_exit, 1) == -1)
                return is_error("Bad semop W_exit\n");
        }
    }

    struct sembuf V_kill = {sem_kill, 1, 0};
    struct sembuf V_access_V_trap[2] = {{sem_access, 1, 0}, {trap, 1, 0}};

    for(int n_pas = 0; n_pas < n_passangers; n_pas++)
    {
        if(semop(semid, &V_kill, 1) == -1)
            return is_error("Bad semop V_kill\n");
        if(semop(semid, V_access_V_trap, 2) == -1)
            return is_error("Bad semop V_access\n");
    }

    return 0;
}

int passenger(int n_passenger)
{
    struct sembuf P_access_P_trap[2] = {{sem_access, -1, 0}, {trap, -1, 0}};
    struct sembuf P_exit_P_trap[2] = {{sem_exit, -1, 0}, {trap, -1, 0}};
    struct sembuf V_trap = {trap, 1, 0};
    struct sembuf P_kill_no_wait = {sem_kill, -1, IPC_NOWAIT};

    while(1)
    {
        if(semop(semid, P_access_P_trap, 2) == -1)
            return is_error("Bad semop P_access_P_trap\n");

        if(semop(semid, &P_kill_no_wait, 1) != -1)
            return 0;

        dump("Пассажир %d отправился по трапу\n", n_passenger);
        usleep(delay*(rand()%100));

        if(semop(semid, &V_trap, 1) == -1)
            return is_error("Bad semop V_trap\n");

        dump("Пассажир %d в лодке\n", n_passenger);

        if(semop(semid, P_exit_P_trap, 2) == -1)
            return is_error("Bad semop P_exit_P_trap\n");

        dump("Пассажир %d зашёл на трап\n", n_passenger);
        usleep(delay*(rand()%100));

        if(semop(semid, &V_trap, 1) == -1)
            return is_error("Bad semop V_trap\n");
        
        dump("Пассажир %d сошел с трапа\n", n_passenger);
    }
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    if(argc != 4)
        return is_error("Incorrect arguments\n");

    int n_trips = atoi(argv[1]);
    int n_passangers = atoi(argv[2]);
    int boat_capacity = atoi(argv[3]);

    //create four semaphores
    semid = semget(IPC_PRIVATE, 4, 0666 | IPC_CREAT);
    if(semid == -1)
        return is_error("Bad semid\n");

    //initializing
    union semun arg;

    arg.val = 2;
    if(semctl(semid, trap, SETVAL, arg) == -1)
        return is_error("Bad SETVAL for trap\n");

    arg.val = 0;
    if(semctl(semid, sem_access, SETVAL, arg) == -1)
        return is_error("Bad SETVAL for sem access\n");

    arg.val = 0;
    if(semctl(semid, sem_exit, SETVAL, arg) == -1)
        return is_error("Bad SETVAL for sem exit\n");

    arg.val = 0;
    if(semctl(semid, sem_kill, SETVAL, arg) == -1)
        return is_error("Bad SETVAL for sem kill\n");

    pid_t boat_pid = fork();
    if(boat_pid == 0)
    {
        return boat(n_trips, boat_capacity, n_passangers);
    }
    else if(boat_pid == -1)
    {
        return is_error("Bad fork\n");
    }

    //run passengers processes
    for(size_t i = 0; i < n_passangers; i++)
    {
        pid_t passanger_pid = fork();
        if(passanger_pid == 0)
        {
            return passenger(i);
        }
        else if(passanger_pid == -1)
        {
            return is_error("Bad fork\n");
        }
    }

    if(waitpid(boat_pid, NULL, 0) == -1)
        return is_error("Bad waitpid\n");

    for(size_t i = 0; i < n_passangers; i++)
        if(wait(NULL) == -1)
            return is_error("Bad wait\n");

    if(semctl(semid, 3, IPC_RMID) == -1)
        return is_error("Bad IPC_RMID for semaphores\n");

    printf("The end\n");
    return 0;
}
