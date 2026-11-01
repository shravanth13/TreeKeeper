#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "treekeeper.h"

// parent of i = (i-1)/2, children = 2i+1 and 2i+2

MinHeap *createHeap(int capacity)
{
    MinHeap *heap = malloc(sizeof(MinHeap));
    if (heap == NULL)
        return NULL;
    heap->arr = malloc(capacity * sizeof(Tree *));
    if (heap->arr == NULL) {
        free(heap);
        return NULL;
    }
    heap->size = 0;
    heap->capacity = capacity;
    return heap;
}

// every tree remembers where it is in the array, so swaps update that too
static void swap(MinHeap *heap, int i, int j)
{
    Tree *temp = heap->arr[i];
    heap->arr[i] = heap->arr[j];
    heap->arr[j] = temp;
    heap->arr[i]->heapIndex = i;
    heap->arr[j]->heapIndex = j;
}

static void heapifyUp(MinHeap *heap, int i)
{
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (hoursLeft(heap->arr[i]) >= hoursLeft(heap->arr[parent]))
            break;
        swap(heap, i, parent);
        i = parent;
    }
}

static void heapifyDown(MinHeap *heap, int i)
{
    while (1) {
        int smallest = i;
        int left = 2 * i + 1;
        int right = 2 * i + 2;

        if (left < heap->size && hoursLeft(heap->arr[left]) < hoursLeft(heap->arr[smallest]))
            smallest = left;
        if (right < heap->size && hoursLeft(heap->arr[right]) < hoursLeft(heap->arr[smallest]))
            smallest = right;
        if (smallest == i)
            break;
        swap(heap, i, smallest);
        i = smallest;
    }
}

// returns 0 if the tree is already in the heap or there is no memory
int heapInsert(MinHeap *heap, Tree *t)
{
    if (t->heapIndex != -1)
        return 0;

    if (heap->size == heap->capacity) {
        Tree **bigger = realloc(heap->arr, 2 * heap->capacity * sizeof(Tree *));
        if (bigger == NULL)
            return 0;
        heap->arr = bigger;
        heap->capacity *= 2;
    }

    heap->arr[heap->size] = t;
    t->heapIndex = heap->size;
    heap->size++;
    heapifyUp(heap, heap->size - 1);
    return 1;
}

Tree *heapPeek(MinHeap *heap)
{
    if (heap->size == 0)
        return NULL;
    return heap->arr[0];
}

Tree *extractMin(MinHeap *heap)
{
    if (heap->size == 0)
        return NULL;
    Tree *min = heap->arr[0];
    heapRemove(heap, min);
    return min;
}

// removes any tree in O(log n) using its stored index
void heapRemove(MinHeap *heap, Tree *t)
{
    int i = t->heapIndex;
    if (i < 0)
        return;

    swap(heap, i, heap->size - 1);
    heap->size--;
    t->heapIndex = -1;

    // the tree moved into position i can be too small or too big for that spot
    if (i < heap->size) {
        Tree *moved = heap->arr[i];
        heapifyUp(heap, i);
        heapifyDown(heap, moved->heapIndex);
    }
}

// call after a tree's water level changes
void heapUpdate(MinHeap *heap, Tree *t)
{
    if (t->heapIndex < 0)
        return;
    heapifyUp(heap, t->heapIndex);
    heapifyDown(heap, t->heapIndex);
}

// O(n), used after the timer changes every water level
void buildHeap(MinHeap *heap)
{
    for (int i = heap->size / 2 - 1; i >= 0; i--)
        heapifyDown(heap, i);
}

// the trees are freed by the hash map
void freeHeap(MinHeap *heap)
{
    if (heap == NULL)
        return;
    free(heap->arr);
    free(heap);
}

// caller must hold sys->lock
void simulateHours(System *sys, float hours)
{
    int n;
    Tree **all = getAllTrees(sys->trees, &n);

    for (int i = 0; i < n; i++) {
        Tree *t = all[i];
        t->water -= t->dryRate * hours;
        if (t->water < 0)
            t->water = 0;

        // an assigned tree that wilts goes back to the queue so it is not forgotten
        if (t->assignedTo != -1 && t->water == 0) {
            t->assignedTo = -1;
            heapInsert(sys->heap, t);
        }
    }
    free(all);

    sys->simHours += hours;
    buildHeap(sys->heap);
}

// timer thread, moves the simulation forward every second
static void *timerLoop(void *arg)
{
    System *sys = arg;

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
    pthread_join(sys->timerThread, NULL);
}
