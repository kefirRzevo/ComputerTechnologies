#include <sys/types.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>

const long MSG_HERE = 1000;

void judge(int n_runners, int qid)
{
    struct timeval  t1, t2, dt;

    fprintf(stdout, "Judge: here\n");
    fprintf(stdout, "Judge: wait for runners\n");

    long msg = 0;
    for (int i = 1; i <= n_runners; i++)
        msgrcv(qid, &msg, 0, MSG_HERE, 0);

    fprintf(stdout, "Judge: runners here\n");

    gettimeofday(&t1, NULL);

    msg = 1;
    msgsnd(qid, &msg, 0, 0);
    msgrcv(qid, &msg, 0, n_runners + 1, 0);

    gettimeofday(&t2, NULL);

    dt.tv_sec  = t2.tv_sec  - t1.tv_sec;
    dt.tv_usec = t2.tv_usec - t1.tv_usec;

    fprintf(stdout, "Judge: runners ended\n");
    fprintf(stdout, "Time: %lf\n", dt.tv_sec+0.000001*dt.tv_usec);
}

void runner(int id, int qid)
{
    fprintf(stdout, "Runner %d: here\n", id);
    long msg = MSG_HERE;
    msgsnd(qid, &msg, 0, 0);

    msgrcv(qid, &msg, 0, id, 0);

    fprintf(stdout, "Runner %d: ran\n", id);

    msg = id + 1;
    msgsnd(qid, &msg, 0, 0);
}

int main(const int argc, const char *argv[])
{
    int qid = msgget(IPC_PRIVATE, IPC_CREAT | 0777);
    int n_runners = atoi(argv[1]);
    assert(n_runners < MSG_HERE);

    int judge_pid = fork();
    if (judge_pid == 0)
    {
        judge(n_runners, qid);
        return 0;
    }

    for (int i = 1; i <= n_runners; i++)
    {
        int runner_pid = fork();
        if(runner_pid == 0)
        {
            runner(i, qid);
            return 0;
        }
    }

    waitpid(judge_pid, NULL, 0);

    for (int i = 1; i <= n_runners; i++)
        wait(NULL);

    msgctl(qid, IPC_RMID, NULL);

    return 0;
}
