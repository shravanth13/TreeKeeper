#ifndef GRAPH_H
#define GRAPH_H

#include "tree.h"

#define INF 1000000000

typedef struct Edge {
    int to;
    int dist;            // metres
    struct Edge *next;
} Edge;

typedef struct {
    char name[NAME_LEN];
    Edge *head;          // adjacency list
} Zone;

typedef struct {
    Zone *zones;
    int count;
    int capacity;
} Graph;

Graph *createGraph(int capacity);
int addZone(Graph *g, const char *name);
int findZone(Graph *g, const char *name);
int addPath(Graph *g, int a, int b, int dist);
int removePath(Graph *g, int a, int b);
void printGraph(Graph *g);
void dijkstra(Graph *g, int src, int dist[], int parent[]);
int shortestPath(Graph *g, int src, int dest);
int planRoute(Graph *g, int start, int stops[], int n);
void freeGraph(Graph *g);

#endif
