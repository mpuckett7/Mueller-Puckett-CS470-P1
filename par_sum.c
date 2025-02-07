/*
 * par_sum.c
 *
 * CS 470 Project 1 (Pthreads)
 * Parallel version
 *
 * Compile with --std=c99
 *
 * Group 13
 */

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

// Struct for singly linked list of numbers.
typedef struct Node
{
    long num;
    struct Node *next;
} Node;

/*
 * returns and dequeues the head of the linked list
 */
Node *dequeue(Node **queue)
{
    Node *ret = *queue;
    *queue = (*queue)->next;
    return ret;
}

/*
 * adds a new node to the end of the linked list
 */
void enqueue(Node **queue, int num)
{
    Node *new_node = (Node *)malloc(sizeof(Node));
    new_node->num = num;
    new_node->next = NULL;

    if (queue && *queue == NULL)
    {
        *queue = new_node;
        return;
    }

    Node *cursor = *queue;
    while (cursor->next != NULL)
    {
        cursor = cursor->next;
    }

    cursor->next = new_node;
}

void free_queue(Node **queue)
{
    Node *cursor = *queue;
    do
    {
        Node *up_next = cursor;
        cursor = cursor->next;
        free(up_next);
    } while (cursor != NULL);
}
// aggregate variables
long sum = 0;
long odd = 0;
long min = INT_MAX;
long max = INT_MIN;
volatile bool done = false;

bool leave = false;
int nthreads = 0;
Node *queue;

pthread_mutex_t use_queue = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t update_info = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t new_task = PTHREAD_COND_INITIALIZER;

// function prototypes
void create_worker(pthread_t *thread);
void *update(void *arg);

/*
 * spawn a new worker thread
 */
void create_worker(pthread_t *thread)
{
    pthread_create(thread, NULL, update, NULL);
}
/*
 * update global aggregate variables given a number
 */
void *update(void *arg)
{
    while (!done || queue)
    {
        int wait_time = 0;

        // All threads but one wait here
        pthread_mutex_lock(&use_queue);

        // If the queue isn't empty, complete the first task in the queue
        if (queue)
        {
            wait_time = queue->num;
            free(dequeue(&queue));
        }
        else
        {
            pthread_cond_wait(&new_task, &use_queue);
            wait_time = queue->num;
            free(dequeue(&queue));
        }
        // unlock mutex
        pthread_mutex_unlock(&use_queue);
        // work here
        sleep(wait_time);

        // update aggregate variables

        // Only one thread updates these at a time.
        pthread_mutex_lock(&update_info);
        sum += wait_time;
        if (wait_time % 2 == 1)
        {
            odd++;
        }
        if (wait_time < min)
        {
            min = wait_time;
        }
        if (wait_time > max)
        {
            max = wait_time;
        }
        pthread_mutex_unlock(&update_info);
        // Thread jumps back in line
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    // check and parse command line options
    if (argc != 3)
    {
        printf("Usage: sum <infile> <nthreads>\n");
        exit(EXIT_FAILURE);
    }
    char *fn = argv[1];

    // open input file
    FILE *fin = fopen(fn, "r");
    if (!fin)
    {
        printf("ERROR: Could not open %s\n", fn);
        exit(EXIT_FAILURE);
    }

    nthreads = strtol(argv[2], NULL, 10);
    if (nthreads <= 0)
    {
        printf("%s\n", "nthreads must be positive.");
        exit(EXIT_FAILURE);
    }
    // DONE READING/CHECKING ARGUMENTS

    char action;
    long num;

    // Create <nthreads> worker threads
    pthread_t threads[nthreads];
    for (int i = 0; i < nthreads; i++)
    {
        create_worker(&threads[i]);
    }
    clock_t start = clock();
    while (fscanf(fin, "%c %ld\n", &action, &num) == 2)
    {

        // check for invalid action parameters
        if (num < 1)
        {
            printf("ERROR: Invalid action parameter: %ld\n", num);
            exit(EXIT_FAILURE);
        }


        pthread_mutex_lock(&use_queue);
        if (action == 'p')
        { // process
            enqueue(&queue, num);
            pthread_cond_signal(&new_task);
        }
        else if (action == 'w')
        { // wait
            sleep(num);
        }
        else
        {
            printf("ERROR: Unrecognized action: '%c'\n", action);
            exit(EXIT_FAILURE);
        }
        pthread_mutex_unlock(&use_queue);
    }
    fclose(fin);
    done = true;
    // wake any idle workers
    pthread_cond_broadcast(&new_task);
    // wait for all workers to finish
    for (int i = 0; i < nthreads; i++)
    {
        pthread_join(threads[i], NULL);
    }
    clock_t clock_time = clock() - start;
    // print results
    printf("%ld %ld %ld %ld\n", sum, odd, min, max);
    printf("%f\n", (double)clock_time / CLOCKS_PER_SEC);

    // clean up and return
    return (EXIT_SUCCESS);
}
