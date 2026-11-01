#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "treekeeper.h"

void clearScreen(void)
{
    printf("\033[2J\033[H");
}

static const char *statusColor(Tree *t)
{
    if (t->water < 30)
        return RED;
    if (t->diseased)
        return MAGENTA;
    if (t->water < 60)
        return YELLOW;
    return GREEN;
}

void printTableHeader(void)
{
    printf(BOLD "  %-5s %-9s %-21s %-19s %-8s %-9s %s\n" RESET,
           "ID", "Species", "Zone", "Water", "Hrs left", "Status", "Assigned");
}

void printTreeRow(System *sys, Tree *t)
{
    Volunteer *v = findVolunteer(sys->vols, t->assignedTo);
    int filled = (int)(t->water / 10 + 0.5);

    printf("  %-5d %-9s %-21s %s[", t->id, t->species, sys->map->zones[t->zone].name, statusColor(t));
    for (int i = 0; i < 10; i++)
        printf(i < filled ? "#" : "-");
    printf("] %5.1f%%" RESET, t->water);
    printf(" %7.1f  %s%-9s" RESET " %s\n", hoursLeft(t), statusColor(t), getStatus(t),
           v != NULL ? v->name : "-");
}

void showDashboard(System *sys)
{
    pthread_mutex_lock(&sys->lock);

    int day = (int)(sys->simHours / 24) + 1;
    int hour = (int)sys->simHours % 24;

    printf(BOLD GREEN "\n=============== TREEKEEPER DASHBOARD ===============\n" RESET);
    printf("  Time: Day %d, %02d:00   Speed: %.2f hrs/sec   %s\n", day, hour, sys->speed,
           sys->paused ? YELLOW "[PAUSED]" RESET : GREEN "[RUNNING]" RESET);

    int n, healthy = 0, thirsty = 0, critical = 0, wilted = 0, diseased = 0, assigned = 0;
    Tree **all = getAllTrees(sys->trees, &n);
    for (int i = 0; i < n; i++) {
        if (all[i]->diseased)
            diseased++;
        if (all[i]->assignedTo != -1)
            assigned++;

        const char *s = getStatus(all[i]);
        if (strcmp(s, "HEALTHY") == 0)
            healthy++;
        else if (strcmp(s, "THIRSTY") == 0)
            thirsty++;
        else if (strcmp(s, "CRITICAL") == 0)
            critical++;
        else if (strcmp(s, "WILTED") == 0)
            wilted++;
    }
    free(all);

    printf("  Trees: %d   " GREEN "Healthy %d  " YELLOW "Thirsty %d  " RED "Critical %d  Wilted %d  "
           MAGENTA "Diseased %d" RESET "\n", n, healthy, thirsty, critical, wilted, diseased);
    printf("  Waiting in queue: %d   Assigned to volunteers: %d\n", sys->heap->size, assigned);
    if (critical + wilted > 0)
        printf(RED BOLD "  ALERT: %d tree(s) need water urgently!\n" RESET, critical + wilted);

    // top 5 from the heap. picked by a simple search so the real heap is not changed
    printf(BOLD CYAN "\n  Most urgent trees (from min heap):\n" RESET);
    if (sys->heap->size == 0) {
        printf("  Queue is empty.\n");
    } else {
        int size = sys->heap->size;
        int used[size];
        for (int i = 0; i < size; i++)
            used[i] = 0;

        printTableHeader();
        for (int r = 0; r < 5 && r < size; r++) {
            int best = -1;
            for (int i = 0; i < size; i++) {
                if (!used[i] && (best == -1 || hoursLeft(sys->heap->arr[i]) < hoursLeft(sys->heap->arr[best])))
                    best = i;
            }
            used[best] = 1;
            printTreeRow(sys, sys->heap->arr[best]);
        }
    }

    printf(BOLD CYAN "\n  Volunteer leaderboard:\n" RESET);
    printLeaderboard(sys->vols);
    printf(BOLD GREEN "====================================================\n" RESET);

    pthread_mutex_unlock(&sys->lock);
}

// reads a whole line. if the line was too long the rest is thrown away
static int readLine(const char *prompt, char *line, int size)
{
    printf("%s", prompt);
    if (fgets(line, size, stdin) == NULL)
        return 0;
    if (strchr(line, '\n') == NULL) {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF)
            ;
    }
    return 1;
}

int readInt(const char *prompt, int *value)
{
    char line[100], extra;
    if (!readLine(prompt, line, sizeof(line)))
        return 0;
    return sscanf(line, "%d %c", value, &extra) == 1;
}

int readFloat(const char *prompt, float *value)
{
    char line[100], extra;
    if (!readLine(prompt, line, sizeof(line)))
        return 0;
    if (sscanf(line, "%f %c", value, &extra) != 1)
        return 0;
    return !isnan(*value) && !isinf(*value);   // sscanf accepts "nan" and "inf"
}

// one word: spaces become '_' ("main gate" -> "main_gate"), commas are not
// allowed because they would break the CSV files
int readString(const char *prompt, char *buf, int size)
{
    char line[100];
    if (!readLine(prompt, line, sizeof(line)))
        return 0;
    line[strcspn(line, "\r\n")] = '\0';

    char *word = line;
    while (*word == ' ' || *word == '\t')
        word++;
    int len = strlen(word);
    while (len > 0 && (word[len - 1] == ' ' || word[len - 1] == '\t'))
        word[--len] = '\0';

    if (len == 0 || len >= size)
        return 0;
    for (int i = 0; i < len; i++) {
        if (word[i] == ',')
            return 0;
        if (word[i] == ' ' || word[i] == '\t')
            word[i] = '_';
    }
    strcpy(buf, word);
    return 1;
}
