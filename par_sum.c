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

// function prototypes
pthread_t *create_worker();
void update(long number);

/*
 * spawn a new worker thread
 */
pthread_t *create_worker(long number)
{
    pthread_t thread;
    pthread_create(&thread, NULL, update, number);
    return &thread;
}
/*
 * update global aggregate variables given a number
 */
void update(long number)
{
    while (1)
    {
        // lock mutex
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
            } else {
                last->next = &n;
                last = &n;
            }
            update(num);
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

    // print results
    printf("%ld %ld %ld %ld\n", sum, odd, min, max);

    // clean up and return
    return (EXIT_SUCCESS);
}
