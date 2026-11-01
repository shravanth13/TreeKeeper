#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "treekeeper.h"

// reduces the water of every tree as if 'hours' have passed
// the caller must hold sys->lock
void simulateHours(System *sys, float hours)
{
    if (sys == NULL || hours <= 0 || isnan(hours) || isinf(hours))
        return;

    int n;
    Tree **all = getAllTrees(sys->trees, &n);
    if (all != NULL) {
        for (int i = 0; i < n; i++) {
            all[i]->water -= all[i]->dryRate * hours;
            if (all[i]->water < 0)
                all[i]->water = 0;

            // If an assigned tree completely wilts before being watered,
            // unassign it and return it to the urgent heap so it is not lost in limbo
            if (all[i]->assignedTo != -1 && all[i]->water <= 0) {
                all[i]->assignedTo = -1;
                heapInsert(sys->heap, all[i]);
            }
        }
        free(all);
    }

    sys->simHours += hours;
    buildHeap(sys->heap);   // priorities changed, so fix the heap
}

// background thread: every second, move the simulation forward
static void *timerLoop(void *arg)
{
    System *sys = (System *)arg;

    while (1) {
        sleep(1);

        pthread_mutex_lock(&sys->lock);
        if (!sys->running) {
            pthread_mutex_unlock(&sys->lock);
            break;
        }
        if (!sys->paused)
            simulateHours(sys, sys->speed);
        pthread_mutex_unlock(&sys->lock);
    }
    return NULL;
}

void startTimer(System *sys)
{
    sys->running = 1;
    if (pthread_create(&sys->timerThread, NULL, timerLoop, sys) != 0) {
        printf("Could not start the timer thread, simulation will not run.\n");
        sys->running = 0;
    }
}

void stopTimer(System *sys)
{
    if (!sys->running)
        return;

    pthread_mutex_lock(&sys->lock);
    sys->running = 0;
    pthread_mutex_unlock(&sys->lock);

    pthread_join(sys->timerThread, NULL);   // wait for the thread to finish
}
