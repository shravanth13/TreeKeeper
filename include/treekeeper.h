#ifndef TREEKEEPER_H
#define TREEKEEPER_H

#include <pthread.h>
#include "tree.h"
#include "heap.h"
#include "graph.h"
#include "volunteer.h"

#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"

typedef struct {
    HashMap *trees;          // all trees, searched by id
    MinHeap *heap;           // trees waiting for water (not assigned)
    Graph *map;
    VolunteerList *vols;

    int nextTreeId;
    int nextVolunteerId;
    float simHours;
    float speed;             // simulated hours per real second

    int paused;
    int running;
    pthread_t timerThread;
    pthread_mutex_t lock;    // menu and timer thread both use the trees and heap
} System;

// heap.c (timer)
void simulateHours(System *sys, float hours);
void startTimer(System *sys);
void stopTimer(System *sys);

// fileio.c
int saveData(System *sys, const char *folder);
int loadData(System *sys, const char *folder);
void loadSampleData(System *sys);

// ui.c
void clearScreen(void);
void printTableHeader(void);
void printTreeRow(System *sys, Tree *t);
void showDashboard(System *sys);
int readInt(const char *prompt, int *value);
int readFloat(const char *prompt, float *value);
int readString(const char *prompt, char *buf, int size);

#endif
