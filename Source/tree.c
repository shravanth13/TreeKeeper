#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "tree.h"

Species speciesList[MAX_SPECIES] = {
    {"Neem", 1.2}, {"Banyan", 1.6}, {"Peepal", 1.5}, {"Mango", 2.2},
    {"Teak", 1.8}, {"Bamboo", 2.8}, {"Gulmohar", 2.0}, {"Arjuna", 1.7}
};
int speciesCount = 8;

static int findSpecies(const char *name)
{
    for (int i = 0; i < speciesCount; i++) {
        if (strcasecmp(name, speciesList[i].name) == 0)
            return i;
    }
    return -1;
}

// returns -1 if the species is not known
float getDryRate(const char *name)
{
    int i = findSpecies(name);
    if (i == -1)
        return -1;
    return speciesList[i].dryRate;
}

int addSpecies(const char *name, float dryRate)
{
    if (findSpecies(name) != -1 || speciesCount == MAX_SPECIES)
        return 0;
    if (dryRate <= 0 || dryRate > 100 || strlen(name) >= NAME_LEN)
        return 0;

    strcpy(speciesList[speciesCount].name, name);
    speciesList[speciesCount].dryRate = dryRate;
    speciesCount++;
    return 1;
}

void printSpeciesList(void)
{
    printf("Available species:\n");
    for (int i = 0; i < speciesCount; i++)
        printf("  %-10s (loses %.1f%% water per hour)\n", speciesList[i].name, speciesList[i].dryRate);
}

Tree *createTree(int id, const char *species, int zone, float water)
{
    int s = findSpecies(species);
    if (s == -1)
        return NULL;

    Tree *t = malloc(sizeof(Tree));
    if (t == NULL)
        return NULL;

    t->id = id;
    strcpy(t->species, speciesList[s].name);   // so "neem" is stored as "Neem"
    t->zone = zone;
    t->water = water;
    t->dryRate = speciesList[s].dryRate;
    t->diseased = 0;
    t->assignedTo = -1;
    t->heapIndex = -1;
    return t;
}

// priority used by the heap, smaller = more urgent
float hoursLeft(Tree *t)
{
    return t->water / t->dryRate;
}

const char *getStatus(Tree *t)
{
    if (t->water <= 0)
        return "WILTED";
    if (t->diseased)
        return "DISEASED";
    if (t->water < 30)
        return "CRITICAL";
    if (t->water < 60)
        return "THIRSTY";
    return "HEALTHY";
}

static int hash(int id, int size)
{
    if (id < 0)
        id = -id;
    return id % size;
}

HashMap *createHashMap(int size)
{
    HashMap *map = malloc(sizeof(HashMap));
    if (map == NULL)
        return NULL;
    map->buckets = calloc(size, sizeof(Node *));
    if (map->buckets == NULL) {
        free(map);
        return NULL;
    }
    map->size = size;
    map->count = 0;
    return map;
}

// doubles the table and moves every node to its new bucket
static int rehash(HashMap *map)
{
    int newSize = map->size * 2;
    Node **newBuckets = calloc(newSize, sizeof(Node *));
    if (newBuckets == NULL)
        return 0;

    for (int i = 0; i < map->size; i++) {
        Node *curr = map->buckets[i];
        while (curr != NULL) {
            Node *next = curr->next;
            int j = hash(curr->tree->id, newSize);
            curr->next = newBuckets[j];
            newBuckets[j] = curr;
            curr = next;
        }
    }
    free(map->buckets);
    map->buckets = newBuckets;
    map->size = newSize;
    return 1;
}

// returns 0 if the id already exists or malloc fails
int hashInsert(HashMap *map, Tree *t)
{
    if (hashSearch(map, t->id) != NULL)
        return 0;
    // keep load factor below 0.75
    if (map->count + 1 > map->size * 3 / 4 && !rehash(map))
        return 0;

    Node *node = malloc(sizeof(Node));
    if (node == NULL)
        return 0;
    int i = hash(t->id, map->size);
    node->tree = t;
    node->next = map->buckets[i];
    map->buckets[i] = node;
    map->count++;
    return 1;
}

Tree *hashSearch(HashMap *map, int id)
{
    for (Node *curr = map->buckets[hash(id, map->size)]; curr != NULL; curr = curr->next) {
        if (curr->tree->id == id)
            return curr->tree;
    }
    return NULL;
}

// removes the node only, the tree itself is not freed
int hashDelete(HashMap *map, int id)
{
    int i = hash(id, map->size);
    Node *curr = map->buckets[i];
    Node *prev = NULL;

    while (curr != NULL) {
        if (curr->tree->id == id) {
            if (prev == NULL)
                map->buckets[i] = curr->next;
            else
                prev->next = curr->next;
            free(curr);
            map->count--;
            return 1;
        }
        prev = curr;
        curr = curr->next;
    }
    return 0;
}

// caller has to free the returned array
Tree **getAllTrees(HashMap *map, int *n)
{
    *n = 0;
    if (map->count == 0)
        return NULL;
    Tree **list = malloc(map->count * sizeof(Tree *));
    if (list == NULL)
        return NULL;

    for (int i = 0; i < map->size; i++) {
        for (Node *curr = map->buckets[i]; curr != NULL; curr = curr->next)
            list[(*n)++] = curr->tree;
    }
    return list;
}

// also frees the trees
void freeHashMap(HashMap *map)
{
    if (map == NULL)
        return;
    for (int i = 0; i < map->size; i++) {
        Node *curr = map->buckets[i];
        while (curr != NULL) {
            Node *next = curr->next;
            free(curr->tree);
            free(curr);
            curr = next;
        }
    }
    free(map->buckets);
    free(map);
}
