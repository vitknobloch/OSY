#include "gen.h"

#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    //Prepare pipe
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        exit(2);
    }

    //gen process fork
    pid_t genpid = fork();
    if (genpid == 0)
    {
        //GEN process
        //close read end of pipe
        if (close(pipefd[0]))
            exit(2);
        if (dup2(pipefd[1], STDOUT_FILENO) != STDOUT_FILENO)
            exit(2);
        if (close(pipefd[1]))
            exit(2);
        if (signal(SIGTERM, stopgen) == SIG_ERR)
            exit(2);

        rungen(4096);
        return 0;
    }
    else if (genpid == -1)
    {
        exit(2);
    }

    //nsd process fork
    pid_t nsdpid = fork();
    if (nsdpid == 0)
    {
        //NSD process
        if (close(pipefd[1]))
            exit(2);
        if (dup2(pipefd[0], STDIN_FILENO) != STDIN_FILENO)
            exit(2);
        if (close(pipefd[0]))
            exit(2);
        if (execl("./nsd", "", NULL))
            exit(2);
    }
    else if (nsdpid == -1)
    {
        kill(genpid, SIGTERM);
        exit(2);
    }

    //main process
    //close pipe
    if (close(pipefd[0]))
        exit(2);
    if (close(pipefd[1]))
        exit(2);

    sleep(5); //wait 5 seconds
    if (kill(genpid, SIGTERM) != 0)
        exit(2);

    int genstat, nsdstat; //return codes of child processes
    pid_t waitgen = waitpid(genpid, &genstat, 0);
    if (waitgen != genpid)
    {
        exit(2);
    }

    pid_t waitnsd = waitpid(nsdpid, &nsdstat, 0);
    if (waitnsd != nsdpid)
    {
        exit(2);
    }

    //check return codes
    if (WEXITSTATUS(genstat) != 0 || WEXITSTATUS(nsdstat) != 0)
    {
        printf("ERROR\n");
        return 1;
    }

    printf("OK\n");
    return 0;
}
