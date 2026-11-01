// Test program for the data structures (run with: make test)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "treekeeper.h"

int passed = 0, failed = 0;

void check(int condition, const char *name)
{
    if (condition) {
        printf("  PASS  %s\n", name);
        passed++;
    } else {
        printf("  FAIL  %s\n", name);
        failed++;
    }
}

void testSpecies(void)
{
    printf("\nSpecies tests:\n");
    check(getDryRate("neem") == getDryRate("Neem"), "species name is not case sensitive");
    check(getDryRate("Coconut") < 0, "unknown species");
    check(addSpecies("Coconut", 2.5) == 1, "add new species");
    check(addSpecies("coconut", 1.0) == 0, "duplicate species rejected");
    check(addSpecies("Jamun", 0) == 0, "zero dry rate rejected");

    Tree *t = createTree(1, "COCONUT", 0, 50);
    check(t != NULL && strcmp(t->species, "Coconut") == 0 && t->dryRate == 2.5f,
          "tree of new species uses the name from the list");
    free(t);
}

void testHashMap(void)
{
    printf("\nHash map tests:\n");
    HashMap *map = createHashMap(4);

    // insert more trees than buckets so that rehashing happens
    for (int i = 1; i <= 100; i++)
        hashInsert(map, createTree(i, "Neem", 0, 50));
    check(map->count == 100, "insert 100 trees");
    check(map->size > 4, "map resized");
    check(hashSearch(map, 57) != NULL && hashSearch(map, 57)->id == 57, "search existing id");
    check(hashSearch(map, 500) == NULL, "search missing id");

    Tree *dup = createTree(10, "Neem", 0, 50);
    check(hashInsert(map, dup) == 0, "duplicate id rejected");
    free(dup);

    Tree *t = hashSearch(map, 10);
    check(hashDelete(map, 10) == 1, "delete id");
    free(t);
    check(hashSearch(map, 10) == NULL, "deleted id not found");
    check(hashDelete(map, 10) == 0, "delete missing id");

    freeHashMap(map);
}

void testHeap(void)
{
    printf("\nMin heap tests:\n");
    MinHeap *heap = createHeap(2);
    float levels[] = {80, 20, 55, 5, 99, 40, 70};
    Tree *trees[7];

    for (int i = 0; i < 7; i++) {
        trees[i] = createTree(i + 1, "Neem", 0, levels[i]);   // same species, so order = water
        heapInsert(heap, trees[i]);
    }
    check(heap->size == 7, "insert 7 trees (heap grows)");
    check(heapPeek(heap)->water == 5, "minimum at root");

    trees[4]->water = 1;            // tree with 99% becomes most urgent
    heapUpdate(heap, trees[4]);
    check(heapPeek(heap) == trees[4], "update moves tree to root");

    heapRemove(heap, trees[3]);     // remove the 5% tree
    check(heap->size == 6 && trees[3]->heapIndex == -1, "remove from middle");

    int sorted = 1;
    float prev = -1;
    while (heap->size > 0) {
        Tree *t = extractMin(heap);
        if (t->water < prev)
            sorted = 0;
        prev = t->water;
    }
    check(sorted, "extractMin returns trees in order");
    check(extractMin(heap) == NULL, "extract from empty heap");

    for (int i = 0; i < 7; i++)
        free(trees[i]);
    freeHeap(heap);
}

void testGraph(void)
{
    printf("\nGraph / Dijkstra tests:\n");
    Graph *g = createGraph(1);
    addZone(g, "A");
    addZone(g, "B");
    addZone(g, "C");
    addZone(g, "D");
    check(g->count == 4, "add 4 zones (graph grows)");
    check(addZone(g, "a") == -1, "duplicate zone rejected");

    addPath(g, 0, 1, 4);   // A-B 4
    addPath(g, 1, 2, 1);   // B-C 1
    addPath(g, 0, 2, 10);  // A-C 10, D has no paths
    check(addPath(g, 0, 0, 5) == 0, "path to itself rejected");

    int dist[4], parent[4];
    dijkstra(g, 0, dist, parent);
    check(dist[2] == 5, "A to C is 5 (through B)");
    check(parent[2] == 1, "C is reached from B");
    check(dist[3] == INF, "D is unreachable");

    removePath(g, 1, 2);
    dijkstra(g, 0, dist, parent);
    check(dist[2] == 10, "after removing B-C, A to C is 10");

    freeGraph(g);
}

void testTimer(void)
{
    printf("\nTimer tests:\n");
    System sys = {0};
    sys.trees = createHashMap(8); sys.heap = createHeap(4); sys.vols = createVolunteerList(2);

    Tree *a = createTree(1, "Neem", 0, 60);     // 1.2 per hour
    Tree *b = createTree(2, "Bamboo", 0, 90);   // 2.8 per hour
    Tree *c = createTree(3, "Teak", 0, 5);      // assigned, so not in the heap
    c->assignedTo = 7;
    hashInsert(sys.trees, a); heapInsert(sys.heap, a);
    hashInsert(sys.trees, b); heapInsert(sys.heap, b);
    hashInsert(sys.trees, c);

    simulateHours(&sys, 10);
    check(a->water == 48 && b->water == 62, "water goes down by dryRate x hours");
    check(c->water == 0 && c->assignedTo == -1 && c->heapIndex != -1, "wilted assigned tree back in queue");
    check(heapPeek(sys.heap) == c, "heap rebuilt after update");
    check(sys.simHours == 10, "time moved forward");

    freeHeap(sys.heap); freeHashMap(sys.trees); freeVolunteerList(sys.vols);
}

// small campus used only for the save / load tests
void loadSampleData(System *sys)
{
    char *zones[] = {
        "Main_Gate", "Admin_Block", "Library", "Lecture_Hall_Complex", "Orion",
        "Octagon", "Hostel_Zone", "Sports_Complex", "Golden_Jubilee", "Pond_Area"
    };
    int paths[13][3] = {
        {0, 1, 450}, {1, 2, 300}, {1, 8, 350}, {2, 3, 250}, {2, 5, 400},
        {3, 4, 200}, {4, 5, 300}, {5, 6, 600}, {6, 7, 350}, {7, 9, 500},
        {8, 9, 700}, {8, 3, 550}, {0, 9, 1200}
    };
    char *species[] = {
        "Neem", "Banyan", "Mango", "Bamboo", "Peepal", "Teak", "Gulmohar", "Arjuna",
        "Neem", "Bamboo", "Mango", "Banyan", "Teak", "Gulmohar", "Peepal", "Arjuna"
    };
    int treeZone[] = {0, 1, 2, 2, 3, 4, 5, 6, 6, 7, 8, 9, 9, 0, 7, 1};
    float water[] = {92, 71, 34, 48, 65, 22, 57, 15, 80, 28, 44, 88, 39, 12, 96, 51};

    for (int i = 0; i < 10; i++)
        addZone(sys->map, zones[i]);
    for (int i = 0; i < 13; i++)
        addPath(sys->map, paths[i][0], paths[i][1], paths[i][2]);

    addVolunteer(sys->vols, sys->nextVolunteerId++, "Shaurya", 6, 0);
    addVolunteer(sys->vols, sys->nextVolunteerId++, "Shravanth", 2, 0);
    addVolunteer(sys->vols, sys->nextVolunteerId++, "Shreya", 1, 0);

    for (int i = 0; i < 16; i++) {
        Tree *t = createTree(sys->nextTreeId, species[i], treeZone[i], water[i]);
        hashInsert(sys->trees, t);
        heapInsert(sys->heap, t);
        sys->nextTreeId++;
    }
}

void testFiles(void)
{
    printf("\nSave / load tests:\n");
    System a = {0}, b = {0};
    a.trees = createHashMap(8); a.heap = createHeap(8); a.map = createGraph(4); a.vols = createVolunteerList(2);
    b.trees = createHashMap(8); b.heap = createHeap(8); b.map = createGraph(4); b.vols = createVolunteerList(2);
    a.nextTreeId = 1001; a.nextVolunteerId = 1; a.speed = 2;
    pthread_mutex_init(&a.lock, NULL);
    pthread_mutex_init(&b.lock, NULL);

    loadSampleData(&a);
    check(saveData(&a, "object/test_data"), "save sample data");
    check(loadData(&b, "object/test_data"), "load it back");
    check(b.map->count == a.map->count, "same number of zones");
    check(b.trees->count == a.trees->count, "same number of trees");
    check(b.heap->size == a.heap->size, "same number of trees in heap");
    check(b.vols->count == 3, "3 volunteers loaded");
    check(b.nextTreeId == a.nextTreeId, "next tree id restored");
    check(hashSearch(b.trees, 1005) != NULL, "tree 1005 found after load");

    // tree of an added species is saved and loaded back
    Tree *t = createTree(a.nextTreeId, "Coconut", 0, 70);
    hashInsert(a.trees, t);
    heapInsert(a.heap, t);
    a.nextTreeId++;
    System c = {0};
    c.trees = createHashMap(8); c.heap = createHeap(8); c.map = createGraph(4); c.vols = createVolunteerList(2);
    pthread_mutex_init(&c.lock, NULL);
    saveData(&a, "object/test_data");
    loadData(&c, "object/test_data");
    check(hashSearch(c.trees, t->id) != NULL, "tree of added species loaded back");
    freeHeap(c.heap); freeHashMap(c.trees); freeGraph(c.map); freeVolunteerList(c.vols);
    pthread_mutex_destroy(&c.lock);

    freeHeap(a.heap); freeHashMap(a.trees); freeGraph(a.map); freeVolunteerList(a.vols);
    freeHeap(b.heap); freeHashMap(b.trees); freeGraph(b.map); freeVolunteerList(b.vols);
    pthread_mutex_destroy(&a.lock);
    pthread_mutex_destroy(&b.lock);
}

int main(void)
{
    testSpecies();
    testHashMap();
    testHeap();
    testGraph();
    testTimer();
    testFiles();
    printf("\n%d passed, %d failed\n", passed, failed);
    return failed > 0;
}
