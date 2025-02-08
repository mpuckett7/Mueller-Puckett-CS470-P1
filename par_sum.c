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

/*
 * Aanlysis Questions:
 *  1. No, we did not use AI for this project.
 * 
 *  2. We ran bash script experiments in the terminal, with different test cases that included an increasing number of threads per run. 
 *     This number expanded from 1, 2, 4, 8, 16, 32, 64, 128 per test case. We used gettimeofday() from sys/time.h to measure time performance/wall 
 *     clock performance since we are using pthreads.
 *
 *  3.  The worker threads remain in a while loop checking both a queue and a done boolean variable. 
 *      The done variable is not updated until the main thread is done processing the text file. 
 *      The worker threads either take something from the queue and do the task or busy wait on the main thread to signal 
 *      that a new task has been added to the queue via a condition variable.
 *
 *  4. Once the supervisor is completed it will then broadcast instead of signal based on a condition variable that threads wait on if the queue is empty. 
 *     This ensures that if any thread is busy waiting on the condition variable after all the tasks have been processed and dealt out 
 *     that they are reawakened. To prevent any straggler from causing havoc we have the threads check to see if the queue is empty before 
 *     grabbing a task when reawakened from the condition variable. Then we join them all back together in a for loop at the end.
 *
 *  5. We would change the way our enqueue method works by having a priority number associated with each task; when processing a new task we would start 
 *     at the head of the linked list and move it down the linked list while its priority is lesser than the current nodes. 
 *     This would not affect synchronization since the queue would still be accessed by the threads in the same manner.
 *
 *  6. To implement task differentiation you would simply create specialized versions of this process; an array of x-worker threads then y-worker threads 
 *     and z-worker threads each with their own queue to pull from and update method to use. The supervisor thread would then sort incoming tasks 
 *     appropriately. Synchronization would be the same just with three different collections of worker threads to sync at the end rather than one.
 *
 * 
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

pthread_mutex_t update_queue = PTHREAD_MUTEX_INITIALIZER;
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
    int i = pthread_create(thread, NULL, update, NULL);
    if (i != 0){
        fprintf(stderr, "Could not create thread\n");
        exit(EXIT_FAILURE);
    }
}
/*
 * update global aggregate variables given a number
 */
void *update(void *arg)
{
    //possible protection some how
    while (1)
    {
        int wait_time = 0;

        // All threads but one wait here
        pthread_mutex_lock(&use_queue);

        // If the queue isn't empty, complete the first task in the queue
        if (queue)
        {
            wait_time = queue->num;
            free(dequeue(&queue));
            pthread_mutex_unlock(&use_queue);
        }
        else
        {
            pthread_cond_wait(&new_task, &use_queue);
            if(queue){
                wait_time = queue->num;
                free(dequeue(&queue));
            }
            pthread_mutex_unlock(&use_queue);
        }
        // unlock mutex

        // work here
        //sleep(wait_time);

        // update aggregate variables

        // Only one thread updates these at a time.
        pthread_mutex_lock(&update_info);
        sum += wait_time;
        if (wait_time % 2 == 1)
        {
            odd++;
        }
        if (wait_time < min && wait_time != 0)
        {
            min = wait_time;
        }
        if (wait_time > max)
        {
            max = wait_time;
        }
        pthread_mutex_unlock(&update_info);
        // Thread jumps back in line

        pthread_mutex_lock(&use_queue);
        if(done && !queue)
        {
            pthread_mutex_unlock(&use_queue);
            return NULL;
        }
        pthread_mutex_unlock(&use_queue);
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

    while (fscanf(fin, "%c %ld\n", &action, &num) == 2)
    {

        // check for invalid action parameters
        if (num < 1)
        {
            printf("ERROR: Invalid action parameter: %ld\n", num);
            exit(EXIT_FAILURE);
        }


        if (action == 'p')
        { // process
            pthread_mutex_lock(&use_queue);
            enqueue(&queue, num);
            pthread_mutex_unlock(&use_queue);
            pthread_cond_signal(&new_task);      
        }
        else if (action == 'w')
        { // wait
            //sleep(num);
        }
        else
        {
            printf("ERROR: Unrecognized action: '%c'\n", action);
            exit(EXIT_FAILURE);
        }
    }
    fclose(fin);

    pthread_mutex_lock(&use_queue);
    done = true;
    pthread_mutex_unlock(&use_queue);
    // wake any idle workers
    pthread_cond_broadcast(&new_task);
    // wait for all workers to finish
    for (int i = 0; i < nthreads; i++)
    {
        int j = pthread_join(threads[i], NULL);
        if (j != 0) {
            fprintf(stderr, "Could not join a thread\n");
            exit(EXIT_FAILURE);
        }
    }

    // print results
    printf("%ld %ld %ld %ld\n", sum, odd, min, max);

    // clean up and return
    return (EXIT_SUCCESS);
}
