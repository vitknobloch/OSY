#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define INITIAL_WORKERS_CAPACITY 30

/*--------------TYPEDEFS----------------*/
#define _PHASE_COUNT 6
enum place
{
    NUZKY,
    VRTACKA,
    OHYBACKA,
    SVARECKA,
    LAKOVNA,
    SROUBOVAK,
    FREZA,
    _PLACE_COUNT
};

enum product
{
    A,
    B,
    C,
    _PRODUCT_COUNT
};

/* Individual worker */
typedef struct
{
    char *name;
    int ended;
    int workplace;
    pthread_t thread;
} worker_t;

/* List of workers */
typedef struct
{
    int worker_count;
    int worker_capacity;
    worker_t *list;
} workers_t;

/*----------GLOBAL VARIABLES------------*/
const char *place_str[_PLACE_COUNT] = {
    [NUZKY] = "nuzky",
    [VRTACKA] = "vrtacka",
    [OHYBACKA] = "ohybacka",
    [SVARECKA] = "svarecka",
    [LAKOVNA] = "lakovna",
    [SROUBOVAK] = "sroubovak",
    [FREZA] = "freza",
};

const int place_wait[_PLACE_COUNT] = {
    [NUZKY] = 100,
    [VRTACKA] = 200,
    [OHYBACKA] = 150,
    [SVARECKA] = 300,
    [LAKOVNA] = 400,
    [SROUBOVAK] = 250,
    [FREZA] = 500,
};

const char *
    product_str[_PRODUCT_COUNT] = {
        [A] = "A",
        [B] = "B",
        [C] = "C",
};

const int production_steps[_PRODUCT_COUNT][_PHASE_COUNT] = {
    [A] = {NUZKY, VRTACKA, OHYBACKA, SVARECKA, VRTACKA, LAKOVNA},
    [B] = {VRTACKA, NUZKY, FREZA, VRTACKA, LAKOVNA, SROUBOVAK},
    [C] = {FREZA, VRTACKA, SROUBOVAK, VRTACKA, FREZA, LAKOVNA},
};

int ready_places[_PLACE_COUNT];
int total_places[_PLACE_COUNT];

//Available parts
int parts[_PRODUCT_COUNT][_PHASE_COUNT];
//Parts aquired by a worker
int parts_in_production[_PRODUCT_COUNT][_PHASE_COUNT];
//Total number of workers for each workplace
int total_workers[_PLACE_COUNT];

/*---------------MUTEXES----------------*/
pthread_mutex_t mtx_ready_places = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_parts = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_wrkrs = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_working_count = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_places[_PLACE_COUNT];
pthread_cond_t cond_parts[_PLACE_COUNT];
pthread_cond_t cond_end;

/* Worker thread main method */
void *worker(void *arg);
/* Thread safe function in which worker waits for part he can process */
void aquire_part(worker_t *wrkr, int *product, int *phase);
/* Thread safe function in which worker waits for workplace where he can process aquired part */
void aquire_workplace(worker_t *wrkr);
/* Thread safe function to check whether worker hasn't been ended */
int continue_working(worker_t *wrkr);

/* Returns index of 'what' in 'array' or -1 */
int find_string_in_array(const char **array, int length, char *what);
/* Extends maximal number of workers if needed */
void reallocate_workers(workers_t *wrkrs, int new_capacity);
/* Returns index of worker with given name in workers or -1 */
int find_worker(const char *name, workers_t *wrkrs);
/* Thread safe function to check if any work can be done by any of the workers*/
void wait_for_end();
/* Thread safe function to update ready_places on place_index by update_count */
void add_to_place_async(int place_index, int update_count);
/* Thread safe function to update parts and parts_in production
   Adds parts_c to parts on position given by product and parts_phase 
   Subtracts parts_c from parts_in_production on position given by product and in_prod_phase */
void add_to_parts_async(int product, int parts_phase, int in_prod_phase, int parts_c);

/* Functions to process individual commands */
void cmd_start(char *arg1, char *arg2, workers_t *wrkrs);
void cmd_make(char *arg1);
void cmd_end(char *arg1, workers_t *wrkrs);
void cmd_add(char *arg1);
void cmd_remove(char *arg1);

int main(int argc, char **argv)
{
    /* Init workers struct*/
    workers_t wrkrs = {.worker_count = 0, .worker_capacity = INITIAL_WORKERS_CAPACITY};
    wrkrs.list = malloc(sizeof(worker_t) * wrkrs.worker_capacity);

    /* Init conditional variables */
    pthread_cond_init(&cond_end, NULL);
    for (int i = 0; i < _PLACE_COUNT; ++i)
    {
        pthread_cond_init(cond_places + i, NULL);
        pthread_cond_init(cond_parts + i, NULL);
    }

    while (1)
    {
        /* Load input */
        char *line, *cmd, *arg1, *arg2, *arg3, *saveptr;
        int s = scanf(" %m[^\n]", &line);
        if (s == EOF)
            break;
        if (s == 0)
            continue;

        /* Split command */
        cmd = strtok_r(line, " ", &saveptr);
        arg1 = strtok_r(NULL, " ", &saveptr);
        arg2 = strtok_r(NULL, " ", &saveptr);
        arg3 = strtok_r(NULL, " ", &saveptr);

        /* Process command */
        if (strcmp(cmd, "start") == 0 && arg1 && arg2 && !arg3)
            cmd_start(arg1, arg2, &wrkrs);
        else if (strcmp(cmd, "make") == 0 && arg1 && !arg2)
            cmd_make(arg1);
        else if (strcmp(cmd, "end") == 0 && arg1 && !arg2)
            cmd_end(arg1, &wrkrs);
        else if (strcmp(cmd, "add") == 0 && arg1 && !arg2)
            cmd_add(arg1);
        else if (strcmp(cmd, "remove") == 0 && arg1 && !arg2)
            cmd_remove(arg1);
        else
            fprintf(stderr, "Invalid command: %s\n", line);
        free(line);
    }

    /* Wait until noone can work */
    wait_for_end();

    /* Mark all workers as ended */
    pthread_mutex_lock(&mtx_wrkrs);
    for (int i = 0; i < wrkrs.worker_count; ++i)
    {
        wrkrs.list[i].ended = 1;
    }
    pthread_mutex_unlock(&mtx_wrkrs);

    /* Unlock all blocked threads */
    for (int i = 0; i < _PLACE_COUNT; ++i)
    {
        pthread_cond_broadcast(cond_places + i);
        pthread_cond_broadcast(cond_parts + i);
    }

    /* Join all worker threads */
    for (int i = 0; i < wrkrs.worker_count; ++i)
    {
        pthread_join(wrkrs.list[i].thread, NULL);
    }

    /* Destroy mutexes */
    pthread_mutex_destroy(&mtx_wrkrs);
    pthread_mutex_destroy(&mtx_ready_places);
    pthread_mutex_destroy(&mtx_parts);
    pthread_mutex_destroy(&mtx_working_count);
    /* Destroy conditional variables */
    pthread_cond_destroy(&cond_end);
    for (int i = 0; i < _PLACE_COUNT; ++i)
    {
        pthread_cond_destroy(cond_places + i);
        pthread_cond_destroy(cond_parts + i);
    }
    /* Free allocated memory */
    for (int i = 0; i < wrkrs.worker_count; ++i)
    {
        free(wrkrs.list[i].name);
    }
    free(wrkrs.list);

    return 0;
}

void *worker(void *arg)
{
    worker_t *wrkr = (worker_t *)arg;

    while (continue_working(wrkr))
    {
        int product, phase;
        aquire_part(wrkr, &product, &phase);
        /* Release resources if ended mid process */
        if (!continue_working(wrkr))
        {
            if (product == -1)
                return NULL;
            //return part
            add_to_parts_async(product, phase, phase, 1);
            pthread_cond_signal(&cond_parts[wrkr->workplace]);
            return NULL;
        }
        aquire_workplace(wrkr);
        /* Release resources if ended mid process */
        if (!continue_working(wrkr))
        {
            //return part
            add_to_parts_async(product, phase, phase, 1);
            pthread_cond_signal(&cond_parts[wrkr->workplace]);

            //return workplace
            add_to_place_async(wrkr->workplace, 1);
            pthread_cond_signal(&cond_places[wrkr->workplace]);
            return NULL;
        }

        //Print work and sleep for works duration
        printf("%s %s %d %s\n", wrkr->name, place_str[wrkr->workplace], phase + 1, product_str[product]);
        struct timespec req = {.tv_sec = 0, .tv_nsec = 1000000 * place_wait[wrkr->workplace]};
        nanosleep(&req, NULL);

        //Add made product to parts
        phase++;
        if (phase >= _PHASE_COUNT) //product finished
        {
            printf("done %s\n", product_str[product]);
            pthread_mutex_lock(&mtx_parts);
            parts_in_production[product][phase - 1]--;
            pthread_mutex_unlock(&mtx_parts);
        }
        else //product not finished
        {
            add_to_parts_async(product, phase, phase - 1, 1);
            pthread_cond_signal(&cond_parts[production_steps[product][phase]]);
        }
        //release place
        add_to_place_async(wrkr->workplace, 1);

        //signal next step
        pthread_cond_signal(&cond_places[wrkr->workplace]);
    }

    return NULL;
}

void aquire_part(worker_t *wrkr, int *product, int *phase)
{
    *product = -1;
    *phase = -1;
    pthread_mutex_lock(&mtx_parts);
    while (*product == -1)
    {
        // Go through parts in order such that higher priority parts are found first
        for (int ph = _PHASE_COUNT - 1; ph >= 0; --ph)
        {
            for (int pr = 0; pr < _PRODUCT_COUNT; ++pr)
            {
                //Only find parts suitable for this worker
                if (production_steps[pr][ph] == wrkr->workplace)
                {
                    if (parts[pr][ph] > 0)
                    {
                        *product = pr;
                        *phase = ph;
                        parts[pr][ph]--;
                        parts_in_production[pr][ph]++;
                        break;
                    }
                }
            }
            if (*product >= 0)
                break;
        }
        if (*product == -1)
        {
            pthread_cond_signal(&cond_end);
            pthread_cond_wait(&cond_parts[wrkr->workplace], &mtx_parts);
            if (!continue_working(wrkr))
                break;
        }
    }
    pthread_mutex_unlock(&mtx_parts);
}

void aquire_workplace(worker_t *wrkr)
{
    pthread_mutex_lock(&mtx_ready_places);
    while (ready_places[wrkr->workplace] < 1)
    {
        pthread_cond_wait(&cond_places[wrkr->workplace], &mtx_ready_places);
        if (!continue_working(wrkr))
            break;
    }
    ready_places[wrkr->workplace]--;
    pthread_mutex_unlock(&mtx_ready_places);
}

int continue_working(worker_t *wrkr)
{
    pthread_mutex_lock(&mtx_wrkrs);
    int ret = !wrkr->ended;
    pthread_mutex_unlock(&mtx_wrkrs);
    return ret;
}

int find_string_in_array(const char **array, int length, char *what)
{
    for (int i = 0; i < length; i++)
        if (strcmp(array[i], what) == 0)
            return i;
    return -1;
}

void reallocate_workers(workers_t *wrkrs, int new_capacity)
{
    wrkrs->list = realloc(wrkrs->list, sizeof(worker_t) * new_capacity);
    if (!wrkrs->list)
        exit(1);
    wrkrs->worker_capacity = new_capacity;
}

int find_worker(const char *name, workers_t *wrkrs)
{
    for (int i = 0; i < wrkrs->worker_count; ++i)
    {
        if (strcmp(wrkrs->list[i].name, name) == 0)
            return i;
    }
    return -1;
}

void wait_for_end()
{
    int cont = 1;
    pthread_mutex_lock(&mtx_parts);
    while (cont)
    {
        cont = 0;
        for (int pr = 0; pr < _PRODUCT_COUNT; ++pr)
        {
            for (int ph = 0; ph < _PHASE_COUNT; ++ph)
            {
                // Currently ready parts + parts that are currently being made
                int part_count = parts[pr][ph] +
                                 (ph == 0 ? 0 : parts_in_production[pr][ph - 1]);
                int workplace = production_steps[pr][ph];
                // If there are or will be parts and tools and workers to make them
                if (
                    part_count > 0 &&
                    total_workers[workplace] > 0 &&
                    total_places[workplace])
                {
                    cont = 1;
                    break;
                }
            }
            // If any parts are in the last phase of being made
            if (parts_in_production[pr][_PHASE_COUNT - 1] > 0)
                cont = 1;
            if (cont)
                break;
        }
        if (cont)
        {
            pthread_cond_wait(&cond_end, &mtx_parts);
        }
    }
    pthread_mutex_unlock(&mtx_parts);
}

void cmd_start(char *arg1, char *arg2, workers_t *wrkrs)
{
    int workplace = find_string_in_array(place_str, _PLACE_COUNT, arg2);
    if (workplace >= 0)
    {
        pthread_mutex_lock(&mtx_wrkrs);
        if (wrkrs->worker_count >= wrkrs->worker_capacity)
            reallocate_workers(wrkrs, wrkrs->worker_capacity * 2);

        worker_t *wrkr = &wrkrs->list[wrkrs->worker_count++];
        wrkr->name = strdup(arg1);
        wrkr->ended = 0;
        wrkr->workplace = workplace;
        pthread_mutex_unlock(&mtx_wrkrs);
        total_workers[workplace]++;

        pthread_create(&wrkr->thread, NULL, worker, (void *)wrkr);
    }
}

void cmd_make(char *arg1)
{
    int product = find_string_in_array(
        product_str,
        _PRODUCT_COUNT,
        arg1);

    if (product >= 0)
    {
        pthread_mutex_lock(&mtx_parts);
        parts[product][0]++;
        pthread_mutex_unlock(&mtx_parts);
        pthread_cond_signal(&cond_parts[production_steps[product][0]]);
    }
}

void cmd_end(char *arg1, workers_t *wrkrs)
{
    pthread_mutex_lock(&mtx_wrkrs);
    int worker_index = find_worker(arg1, wrkrs);
    if (worker_index >= 0)
    {
        wrkrs->list[worker_index].ended = 1;
    }
    pthread_mutex_unlock(&mtx_wrkrs);
    total_workers[wrkrs->list[worker_index].workplace]--;
    pthread_cond_broadcast(&cond_parts[wrkrs->list[worker_index].workplace]);
    pthread_cond_broadcast(&cond_places[wrkrs->list[worker_index].workplace]);
}

void cmd_add(char *arg1)
{
    int place = find_string_in_array(place_str, _PLACE_COUNT, arg1);
    if (place >= 0)
    {
        add_to_place_async(place, 1);
        total_places[place]++;
        pthread_cond_signal(&cond_places[place]);
    }
}

void cmd_remove(char *arg1)
{
    int place_index = find_string_in_array(place_str, _PLACE_COUNT, arg1);
    if (place_index >= 0)
    {
        add_to_place_async(place_index, -1);
        total_places[place_index]--;
    }
}

void add_to_place_async(int place_index, int update_count)
{
    pthread_mutex_lock(&mtx_ready_places);
    ready_places[place_index] += update_count;
    pthread_mutex_unlock(&mtx_ready_places);
}

void add_to_parts_async(int product, int parts_phase, int in_prod_phase, int parts_c)
{
    pthread_mutex_lock(&mtx_parts);
    parts[product][parts_phase] += parts_c;
    parts_in_production[product][in_prod_phase] -= parts_c;
    pthread_mutex_unlock(&mtx_parts);
}
