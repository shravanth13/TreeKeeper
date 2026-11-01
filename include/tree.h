#ifndef TREE_H
#define TREE_H

#define NAME_LEN 30
#define MAX_SPECIES 50

typedef struct {
    char name[NAME_LEN];
    float dryRate;           // % of water lost per hour
} Species;

typedef struct {
    int id;                  // also the QR code on the tree guard (TK-id)
    char species[NAME_LEN];
    int zone;                // zone index in the graph
    float water;             // 0 - 100 %
    float dryRate;
    int diseased;
    int assignedTo;          // volunteer id, -1 if not assigned
    int heapIndex;           // position in the heap, -1 if not in it
} Tree;

// hash map with separate chaining, key = tree id
typedef struct Node {
    Tree *tree;
    struct Node *next;
} Node;

typedef struct {
    Node **buckets;
    int size;
    int count;
} HashMap;

extern Species speciesList[MAX_SPECIES];
extern int speciesCount;

float getDryRate(const char *name);
int addSpecies(const char *name, float dryRate);
void printSpeciesList(void);

Tree *createTree(int id, const char *species, int zone, float water);
float hoursLeft(Tree *t);
const char *getStatus(Tree *t);

HashMap *createHashMap(int size);
int hashInsert(HashMap *map, Tree *t);
Tree *hashSearch(HashMap *map, int id);
int hashDelete(HashMap *map, int id);
Tree **getAllTrees(HashMap *map, int *n);
void freeHashMap(HashMap *map);

#endif
