#include "gen.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int run = 1;

void rungen(unsigned int num_limit)
{
    while (run)
    {
        printf("%d %d\n", rand() % num_limit, rand() % num_limit);
        fflush(stdout);
        sleep(1);
    }
    fprintf(stderr, "GEN TERMINATED\n");
}

void stopgen(int signo)
{
    run = 0;
}
