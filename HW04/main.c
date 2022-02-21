#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

volatile int consumers_run = 1; //flag stating if input ended
//mutexes and semaphores
sem_t sem_list;
pthread_mutex_t mtx_list;
pthread_mutex_t mtx_stdout;

typedef struct
{
    int x;
    char *word;
    void *next;
} command_t;

typedef struct
{
    command_t *head;
    command_t *tail;
} command_list_t;

typedef struct
{
    int n;
    command_list_t *list;
} consumer_arg_t;

typedef struct
{
    int ret;
    command_list_t *list;
} producer_arg_t;

void *producer(void *arg);
void *consumer(void *arg);
int consumer_continue(command_list_t *list);

int main(int argc, char *argv[])
{
    //process argument
    int n = 1;
    if (argc > 1)
    {
        if (sscanf(argv[1], "%d", &n) != 1)
            n = 0;
    }
    if (n < 1 || n > sysconf(_SC_NPROCESSORS_ONLN))
        exit(1);

    //init list
    command_list_t list = {.head = NULL, .tail = NULL};
    //init semaphores and mutexes
    if (sem_init(&sem_list, 0, 0) != 0)
        exit(1);
    if (pthread_mutex_init(&mtx_list, NULL) != 0)
        exit(1);
    if (pthread_mutex_init(&mtx_stdout, NULL) != 0)
        exit(1);

    //start threads
    pthread_t th_producer;
    pthread_t th_consumers[n];
    consumer_arg_t con_arg[n];
    for (int i = 0; i < n; ++i)
    {
        con_arg[i].n = i + 1;
        con_arg[i].list = &list;
        if (pthread_create(&th_consumers[i], NULL, consumer, &con_arg[i]))
            exit(1);
    }
    producer_arg_t prod_arg = {.list = &list, .ret = 0};
    if (pthread_create(&th_producer, NULL, producer, &prod_arg))
        exit(1);

    //wait for end of input
    if (pthread_join(th_producer, NULL))
        exit(1);

    //mark input as ended
    pthread_mutex_lock(&mtx_list);
    consumers_run = 0;
    pthread_mutex_unlock(&mtx_list);
    //wake all threads
    for (int i = 0; i < n; ++i)
    {
        sem_post(&sem_list);
    }
    //join all threads
    for (int i = 0; i < n; ++i)
    {
        pthread_join(th_consumers[i], NULL);
    }

    //destroy semaphores and mutexes
    sem_destroy(&sem_list);
    pthread_mutex_destroy(&mtx_list);
    pthread_mutex_destroy(&mtx_stdout);

    //return value based on type of input termination (correct or incorrect)
    if (prod_arg.ret != 0)
        return 1;

    return 0;
}

void *producer(void *arg)
{
    command_list_t *list = ((producer_arg_t *)arg)->list;
    int ret, x;
    char *text;
    while ((ret = scanf("%d %ms", &x, &text)) == 2)
    {
        //create command from input
        command_t *cmd = malloc(sizeof(command_t));
        if (!cmd)
            exit(1);
        cmd->next = NULL;
        cmd->word = text;

        //check x correctness
        if (x < 0)
            exit(1);
        cmd->x = x;

        //add command to linked list
        pthread_mutex_lock(&mtx_list);
        if (list->tail)
            list->tail->next = cmd;
        list->tail = cmd;
        if (!list->head)
            list->head = cmd;
        pthread_mutex_unlock(&mtx_list);

        //wake one thread
        sem_post(&sem_list);
    }

    //check input termination correctness and set return value accordingly
    if (ret != 2 && ret != EOF)
    {
        ((producer_arg_t *)arg)->ret = 1;
        return &((producer_arg_t *)arg)->ret;
    }

    return &((producer_arg_t *)arg)->ret;
}

/* Checks id consumer should stop or not
Stops it in case the producer ended and the list is empty*/
int consumer_continue(command_list_t *list)
{
    pthread_mutex_lock(&mtx_list);
    int ret = (consumers_run || list->head);
    pthread_mutex_unlock(&mtx_list);
    return ret;
}

void *consumer(void *arg)
{
    command_list_t *list = ((consumer_arg_t *)arg)->list;
    int n = ((consumer_arg_t *)arg)->n;
    while (consumer_continue(list))
    {
        //waits until producer creates command
        sem_wait(&sem_list);

        //pop command from the list
        pthread_mutex_lock(&mtx_list);
        if (!list->head)
        {
            pthread_mutex_unlock(&mtx_list);
            continue;
        }
        command_t *cmd = list->head;
        list->head = list->head->next;
        if (!list->head)
            list->tail = NULL;
        pthread_mutex_unlock(&mtx_list);

        //process command (print to stdout)
        pthread_mutex_lock(&mtx_stdout);
        printf("Thread %d:", n);
        for (int i = 0; i < cmd->x; ++i)
        {
            printf(" %s", cmd->word);
        }
        printf("\n");
        pthread_mutex_unlock(&mtx_stdout);

        //free command memory
        free(cmd->word);
        free(cmd);
    }
    return NULL;
}