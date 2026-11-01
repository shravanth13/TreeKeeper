#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "graph.h"

Graph *createGraph(int capacity)
{
    Graph *g = malloc(sizeof(Graph));
    if (g == NULL)
        return NULL;
    g->zones = malloc(capacity * sizeof(Zone));
    if (g->zones == NULL) {
        free(g);
        return NULL;
    }
    g->count = 0;
    g->capacity = capacity;
    return g;
}

// returns the new zone's index, or -1 if the name is bad or already used
int addZone(Graph *g, const char *name)
{
    if (strlen(name) == 0 || strlen(name) >= NAME_LEN || findZone(g, name) != -1)
        return -1;

    if (g->count == g->capacity) {
        Zone *bigger = realloc(g->zones, 2 * g->capacity * sizeof(Zone));
        if (bigger == NULL)
            return -1;
        g->zones = bigger;
        g->capacity *= 2;
    }

    strcpy(g->zones[g->count].name, name);
    g->zones[g->count].head = NULL;
    g->count++;
    return g->count - 1;
}

int findZone(Graph *g, const char *name)
{
    for (int i = 0; i < g->count; i++) {
        if (strcasecmp(g->zones[i].name, name) == 0)
            return i;
    }
    return -1;
}

static int addEdge(Graph *g, int from, int to, int dist)
{
    Edge *e = malloc(sizeof(Edge));
    if (e == NULL)
        return 0;
    e->to = to;
    e->dist = dist;
    e->next = g->zones[from].head;
    g->zones[from].head = e;
    return 1;
}

static int deleteEdge(Graph *g, int from, int to)
{
    Edge *curr = g->zones[from].head;
    Edge *prev = NULL;
    while (curr != NULL) {
        if (curr->to == to) {
            if (prev == NULL)
                g->zones[from].head = curr->next;
            else
                prev->next = curr->next;
            free(curr);
            return 1;
        }
        prev = curr;
        curr = curr->next;
    }
    return 0;
}

// undirected, so the edge is stored in both lists.
// if the path already exists only the distance changes
int addPath(Graph *g, int a, int b, int dist)
{
    if (a < 0 || b < 0 || a >= g->count || b >= g->count || a == b || dist <= 0)
        return 0;

    deleteEdge(g, a, b);
    deleteEdge(g, b, a);
    if (!addEdge(g, a, b, dist))
        return 0;
    if (!addEdge(g, b, a, dist)) {
        deleteEdge(g, a, b);
        return 0;
    }
    return 1;
}

int removePath(Graph *g, int a, int b)
{
    if (a < 0 || b < 0 || a >= g->count || b >= g->count)
        return 0;
    int found = deleteEdge(g, a, b);
    deleteEdge(g, b, a);
    return found;
}

void printGraph(Graph *g)
{
    printf("\nCampus map (adjacency list):\n");
    for (int i = 0; i < g->count; i++) {
        printf("  [%d] %-22s ->", i, g->zones[i].name);
        if (g->zones[i].head == NULL)
            printf(" (no paths)");
        for (Edge *e = g->zones[i].head; e != NULL; e = e->next)
            printf(" %s(%dm)", g->zones[e->to].name, e->dist);
        printf("\n");
    }
}

// array version of Dijkstra, O(V^2)
// dist[v] = shortest distance from src, parent[v] = previous zone on that path
void dijkstra(Graph *g, int src, int dist[], int parent[])
{
    int visited[g->count];

    for (int i = 0; i < g->count; i++) {
        dist[i] = INF;
        parent[i] = -1;
        visited[i] = 0;
    }
    dist[src] = 0;

    for (int k = 0; k < g->count; k++) {
        int u = -1;
        for (int i = 0; i < g->count; i++) {
            if (!visited[i] && (u == -1 || dist[i] < dist[u]))
                u = i;
        }
        if (dist[u] == INF)
            break;   // rest of the zones cannot be reached
        visited[u] = 1;

        for (Edge *e = g->zones[u].head; e != NULL; e = e->next) {
            if (!visited[e->to] && dist[u] + e->dist < dist[e->to]) {
                dist[e->to] = dist[u] + e->dist;
                parent[e->to] = u;
            }
        }
    }
}

static void printPath(Graph *g, int parent[], int v)
{
    if (parent[v] == -1) {
        printf("%s", g->zones[v].name);
        return;
    }
    printPath(g, parent, parent[v]);
    printf(" -> %s", g->zones[v].name);
}

// returns the distance, or -1 if dest cannot be reached
int shortestPath(Graph *g, int src, int dest)
{
    int dist[g->count], parent[g->count];

    dijkstra(g, src, dist, parent);
    if (dist[dest] == INF)
        return -1;
    printf("  ");
    printPath(g, parent, dest);
    printf("\n  Total distance: %d m\n", dist[dest]);
    return dist[dest];
}

// greedy walk: from the current zone always go to the nearest stop not visited yet.
// returns the total distance, or -1 if some stop cannot be reached
int planRoute(Graph *g, int start, int stops[], int n)
{
    int dist[g->count], parent[g->count], done[n];
    int current = start, total = 0;

    for (int i = 0; i < n; i++)
        done[i] = 0;
    printf("  Start at %s\n", g->zones[start].name);

    for (int step = 0; step < n; step++) {
        dijkstra(g, current, dist, parent);

        int best = -1;
        for (int i = 0; i < n; i++) {
            if (!done[i] && (best == -1 || dist[stops[i]] < dist[stops[best]]))
                best = i;
        }
        if (dist[stops[best]] == INF) {
            printf("  %s cannot be reached!\n", g->zones[stops[best]].name);
            return -1;
        }

        done[best] = 1;
        if (stops[best] != current) {
            printf("  Walk: ");
            printPath(g, parent, stops[best]);
            printf("  (%d m)\n", dist[stops[best]]);
            total += dist[stops[best]];
            current = stops[best];
        }
    }
    return total;
}

void freeGraph(Graph *g)
{
    if (g == NULL)
        return;
    for (int i = 0; i < g->count; i++) {
        Edge *e = g->zones[i].head;
        while (e != NULL) {
            Edge *next = e->next;
            free(e);
            e = next;
        }
    }
    free(g->zones);
    free(g);
}
