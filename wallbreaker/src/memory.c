#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"

static int strategy_index(const problem_t *p, const char *name)
{
    for (int i = 0; i < p->n_strategies; i++)
        if (!strcmp(p->strategy_names[i], name))
            return i;
    return -1;
}

int memory_load(const char *path, const problem_t *p, wallmap_t *m)
{
    char line[512], word[64];
    int version, wall = -1;
    FILE *f = fopen(path, "r");

    if (!f)
        return 0;
    if (!fgets(line, sizeof line, f) || sscanf(line, "wallbreaker %d", &version) != 1 ||
        !fgets(line, sizeof line, f) || sscanf(line, "problem %63s", word) != 1 ||
        strcmp(word, p->name)) {
        fclose(f);
        return -1;
    }
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\n")] = '\0';
        if (!strncmp(line, "merge ", 6)) {
            m->merge_threshold = atof(line + 6);
        } else if (!strncmp(line, "wall ", 5)) {
            long hits;
            int ntok, off;
            if (sscanf(line, "wall %ld %d%n", &hits, &ntok, &off) != 2)
                continue;
            wall = wallmap_spawn(m, "");
            wall_t *w = &m->walls[wall];
            w->hits = hits;
            char *rest = line + off;
            for (int i = 0; i < ntok && i < WB_MAX_TOKENS; i++) {
                int used;
                if (sscanf(rest, "%15s%n", w->sig.tok[i], &used) != 1)
                    break;
                w->sig.n++;
                rest += used;
            }
        } else if (!strncmp(line, "example ", 8) && wall >= 0) {
            snprintf(m->walls[wall].example, WB_MSG_LEN, "%.*s", WB_MSG_LEN - 1, line + 8);
        } else if (!strncmp(line, "stat ", 5) && wall >= 0) {
            double reward;
            long tries;
            if (sscanf(line, "stat %63s %lf %ld", word, &reward, &tries) != 3)
                continue;
            int s = strategy_index(p, word);
            if (s >= 0) {
                m->walls[wall].reward[s] = reward;
                m->walls[wall].tries[s] = tries;
            }
        } else if (!strcmp(line, "end")) {
            wall = -1;
        }
    }
    fclose(f);
    return 0;
}

int memory_save(const char *path, const problem_t *p, const wallmap_t *m)
{
    FILE *f = fopen(path, "w");
    if (!f)
        return -1;
    fprintf(f, "wallbreaker 1\nproblem %s\nmerge %g\n", p->name, m->merge_threshold);
    for (int i = 0; i < m->n; i++) {
        const wall_t *w = &m->walls[i];
        fprintf(f, "wall %ld %d", w->hits, w->sig.n);
        for (int t = 0; t < w->sig.n; t++)
            fprintf(f, " %s", w->sig.tok[t]);
        fprintf(f, "\nexample %s\n", w->example);
        for (int s = 0; s < p->n_strategies; s++)
            if (w->tries[s])
                fprintf(f, "stat %s %.17g %ld\n", p->strategy_names[s],
                        w->reward[s], w->tries[s]);
        fprintf(f, "end\n");
    }
    return fclose(f);
}
