#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "treekeeper.h"

// files in the data folder:
//   settings.csv    simHours,nextTreeId,nextVolunteerId,speed
//   species.csv     name,dryRate
//   zones.csv       name             (line number = zone index)
//   paths.csv       zoneA,zoneB,distance
//   trees.csv       id,species,zone,water,diseased,assignedTo
//   volunteers.csv  id,name,zone,tasksDone

static FILE *openFile(const char *folder, const char *file, const char *mode)
{
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", folder, file);
    return fopen(path, mode);
}

int saveData(System *sys, const char *folder)
{
    mkdir(folder, 0755);   // fails without harm if it already exists

    FILE *f = openFile(folder, "settings.csv", "w");
    if (f == NULL) {
        printf("Error: could not save in '%s'.\n", folder);
        return 0;
    }

    pthread_mutex_lock(&sys->lock);

    fprintf(f, "%.2f,%d,%d,%.2f\n", sys->simHours, sys->nextTreeId, sys->nextVolunteerId, sys->speed);
    fclose(f);

    f = openFile(folder, "species.csv", "w");
    if (f != NULL) {
        for (int i = 0; i < speciesCount; i++)
            fprintf(f, "%s,%.2f\n", speciesList[i].name, speciesList[i].dryRate);
        fclose(f);
    }

    f = openFile(folder, "zones.csv", "w");
    if (f != NULL) {
        for (int i = 0; i < sys->map->count; i++)
            fprintf(f, "%s\n", sys->map->zones[i].name);
        fclose(f);
    }

    f = openFile(folder, "paths.csv", "w");
    if (f != NULL) {
        for (int i = 0; i < sys->map->count; i++) {
            for (Edge *e = sys->map->zones[i].head; e != NULL; e = e->next) {
                if (i < e->to)   // every path is in both lists, save it once
                    fprintf(f, "%d,%d,%d\n", i, e->to, e->dist);
            }
        }
        fclose(f);
    }

    f = openFile(folder, "trees.csv", "w");
    if (f != NULL) {
        int n;
        Tree **all = getAllTrees(sys->trees, &n);
        for (int i = 0; i < n; i++)
            fprintf(f, "%d,%s,%d,%.2f,%d,%d\n", all[i]->id, all[i]->species, all[i]->zone,
                    all[i]->water, all[i]->diseased, all[i]->assignedTo);
        free(all);
        fclose(f);
    }

    f = openFile(folder, "volunteers.csv", "w");
    if (f != NULL) {
        for (int i = 0; i < sys->vols->count; i++) {
            Volunteer *v = &sys->vols->list[i];
            fprintf(f, "%d,%s,%d,%d\n", v->id, v->name, v->zone, v->tasksDone);
        }
        fclose(f);
    }

    pthread_mutex_unlock(&sys->lock);
    return 1;
}

// puts the tree in the hash map, and in the heap if nobody is assigned to it
static int addTree(System *sys, Tree *t)
{
    if (!hashInsert(sys->trees, t))
        return 0;
    if (t->assignedTo == -1 && !heapInsert(sys->heap, t)) {
        hashDelete(sys->trees, t->id);
        return 0;
    }
    return 1;
}

// returns 0 if there is no saved data. bad lines are skipped
int loadData(System *sys, const char *folder)
{
    char line[200], name[NAME_LEN];
    int skipped = 0;

    FILE *f = openFile(folder, "zones.csv", "r");
    if (f == NULL)
        return 0;
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (strlen(line) > 0 && addZone(sys->map, line) == -1)
            skipped++;
    }
    fclose(f);
    if (sys->map->count == 0)
        return 0;

    f = openFile(folder, "settings.csv", "r");
    if (f != NULL) {
        if (fscanf(f, "%f,%d,%d,%f", &sys->simHours, &sys->nextTreeId, &sys->nextVolunteerId, &sys->speed) != 4)
            skipped++;
        fclose(f);
    }
    if (!(sys->speed > 0 && sys->speed <= 24))   // also catches nan
        sys->speed = 0.05;

    // species before trees, otherwise trees of an added species are skipped
    f = openFile(folder, "species.csv", "r");
    if (f != NULL) {
        float rate;
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "%29[^,],%f", name, &rate) != 2)
                skipped++;
            else if (getDryRate(name) < 0 && !addSpecies(name, rate))
                skipped++;
        }
        fclose(f);
    }

    f = openFile(folder, "paths.csv", "r");
    if (f != NULL) {
        int a, b, d;
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "%d,%d,%d", &a, &b, &d) != 3 || !addPath(sys->map, a, b, d))
                skipped++;
        }
        fclose(f);
    }

    f = openFile(folder, "volunteers.csv", "r");
    if (f != NULL) {
        int id, zone, tasks;
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "%d,%29[^,],%d,%d", &id, name, &zone, &tasks) != 4 ||
                zone < 0 || zone >= sys->map->count ||
                !addVolunteer(sys->vols, id, name, zone, tasks)) {
                skipped++;
                continue;
            }
            if (id >= sys->nextVolunteerId)
                sys->nextVolunteerId = id + 1;
        }
        fclose(f);
    }

    f = openFile(folder, "trees.csv", "r");
    if (f != NULL) {
        int id, zone, diseased, assigned;
        float water;
        while (fgets(line, sizeof(line), f)) {
            if (sscanf(line, "%d,%29[^,],%d,%f,%d,%d", &id, name, &zone, &water, &diseased, &assigned) != 6 ||
                zone < 0 || zone >= sys->map->count || water < 0 || water > 100) {
                skipped++;
                continue;
            }
            Tree *t = createTree(id, name, zone, water);
            if (t == NULL) {
                skipped++;
                continue;
            }
            t->diseased = diseased;
            if (findVolunteer(sys->vols, assigned) != NULL)
                t->assignedTo = assigned;
            if (!addTree(sys, t)) {
                free(t);
                skipped++;
                continue;
            }
            if (id >= sys->nextTreeId)
                sys->nextTreeId = id + 1;
        }
        fclose(f);
    }

    if (skipped > 0)
        printf("Warning: %d invalid line(s) in the data files were skipped.\n", skipped);
    return 1;
}
