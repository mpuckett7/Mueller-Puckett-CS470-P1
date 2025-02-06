/*
 * par_sum.c
 *
 * CS 470 Project 1 (Pthreads)
 * Parallel version
 *
 * Compile with --std=c99
 *
 * Beau Mueller
 * Mason Pucket
 */

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>


// Struct for singly linked list of numbers. 
typedef struct Node Node;
struct Node
{
    long num;
    Node *next;
};
// aggregate variables
long sum = 0;
long odd = 0;
long min = INT_MAX;
long max = INT_MIN;
bool done = false;


bool leave = false;
int nthreads = 0;
Node current;

// function prototypes
void create_worker(pthread_t *thread);
void update(long number);

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
void update(long number)
{
    while (!done)
    {
        /*
         * All threads wait here except for one which locks the mutex and
         * waits in the inner while loop for the queue to be non-empty.
         * After the active thread is alerted of the new task in the queue,
         * it exits the inner while loop, unlocks the mutex and sleeps for
         * x seconds and updates the aggregate variables. After this it
         * waits in line with the rest of the threads.
        */
        while (!leave)
        {
            // cond_wait(&cond, &mut)
        }
        // unlock mutex

        // work here
        sleep(number);

        // update aggregate variables
        sum += number;
        if (number % 2 == 1)
        {
            odd++;
        }
        if (number < min)
        {
            min = number;
        }
        if (number > max)
        {
            max = number;
        }
    }
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


    // load numbers and add them to the queue
    char action;
    long num;
    Node *last;

    // Create <nthreads> worker threads
    pthread_t threads[nthreads];
    for (int i = 0; i < nthreads; i++){
        pthread_t t;
        create_worker(&t);
        threads[i] = t;
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
            struct Node n;
            n.num = num;
            if (!last) {
                last = &n;
                current = *last;
            } else {
                last->next = &n;
                last = &n;
            }
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
    }
    fclose(fin);
    done = true;
    // wake any idle workers
    // wait for all workers to finish

    // print results
    printf("%ld %ld %ld %ld\n", sum, odd, min, max);

    // clean up and return
    return (EXIT_SUCCESS);
}
