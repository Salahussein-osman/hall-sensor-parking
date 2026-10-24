#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "walls.h"

static void add_token(signature_t *s, const char *t)
{
    for (int i = 0; i < s->n; i++)
        if (!strcmp(s->tok[i], t))
            return;
    if (s->n < WB_MAX_TOKENS) {
        snprintf(s->tok[s->n], WB_TOKEN_LEN, "%s", t);
        s->n++;
    }
}

void fingerprint(const char *msg, signature_t *out)
{
    char buf[WB_TOKEN_LEN];
    const unsigned char *p = (const unsigned char *)msg;

    out->n = 0;
    while (*p) {
        if (isdigit(*p)) {
            int len = 0;
            while (isdigit(*p)) {
                len++;
                p++;
            }
            if (*p == '.' && isdigit(p[1]))
                for (p++; isdigit(*p); p++)
                    ;
            snprintf(buf, sizeof buf, "#%d", len);
            add_token(out, buf);
        } else if (isalpha(*p) || *p == '_') {
            int k = 0;
            for (; isalpha(*p) || *p == '_'; p++)
                if (k < WB_TOKEN_LEN - 1)
                    buf[k++] = (char)tolower(*p);
            buf[k] = '\0';
            add_token(out, buf);
        } else {
            p++;
        }
    }
}

double similarity(const signature_t *a, const signature_t *b)
{
    int inter = 0;
    for (int i = 0; i < a->n; i++)
        for (int j = 0; j < b->n; j++)
            if (!strcmp(a->tok[i], b->tok[j])) {
                inter++;
                break;
            }
    int uni = a->n + b->n - inter;
    return uni ? (double)inter / uni : 1.0;
}

/* Order-independent: XOR of each token's FNV-1a. */
uint64_t signature_hash(const signature_t *s)
{
    uint64_t h = 0;
    for (int i = 0; i < s->n; i++) {
        uint64_t t = 1469598103934665603ULL;
        for (const char *c = s->tok[i]; *c; c++)
            t = (t ^ (unsigned char)*c) * 1099511628211ULL;
        h ^= t;
    }
    return h;
}

void wallmap_init(wallmap_t *m, double merge_threshold)
{
    m->walls = NULL;
    m->n = m->cap = 0;
    m->merge_threshold = merge_threshold;
}

void wallmap_free(wallmap_t *m)
{
    free(m->walls);
    wallmap_init(m, m->merge_threshold);
}

int wallmap_match(const wallmap_t *m, const char *msg)
{
    signature_t sig;
    int best = -1;
    double best_sim = 0;

    fingerprint(msg, &sig);
    for (int i = 0; i < m->n; i++) {
        double sim = similarity(&sig, &m->walls[i].sig);
        if (sim > best_sim) {
            best = i;
            best_sim = sim;
        }
    }
    return best_sim >= m->merge_threshold ? best : -1;
}

int wallmap_spawn(wallmap_t *m, const char *msg)
{
    if (m->n == m->cap) {
        int cap = m->cap ? m->cap * 2 : 8;
        wall_t *w = realloc(m->walls, (size_t)cap * sizeof *w);
        if (!w) {
            perror("wallbreaker: realloc");
            exit(1);
        }
        m->walls = w;
        m->cap = cap;
    }
    wall_t *w = &m->walls[m->n];
    memset(w, 0, sizeof *w);
    fingerprint(msg, &w->sig);
    snprintf(w->example, sizeof w->example, "%s", msg);
    return m->n++;
}

int wall_pick(const wall_t *w, int n_strategies, rng_t *rng, double c)
{
    long total = 0;
    int best[WB_MAX_STRATEGIES], n_best = 0;
    double best_score = -INFINITY;

    for (int i = 0; i < n_strategies; i++)
        total += w->tries[i];
    if (!total)
        total = 1;
    for (int i = 0; i < n_strategies; i++) {
        double score = INFINITY;
        if (w->tries[i])
            score = w->reward[i] / w->tries[i] +
                    c * sqrt(log((double)total) / w->tries[i]);
        if (score > best_score) {
            best_score = score;
            n_best = 0;
        }
        if (score == best_score)
            best[n_best++] = i;
    }
    return best[rng_below(rng, n_best)];
}

void wall_record(wall_t *w, int strategy, double reward)
{
    w->reward[strategy] += reward;
    w->tries[strategy]++;
}

int wall_best(const wall_t *w, int n_strategies)
{
    int best = -1;
    double best_rate = -1;
    for (int i = 0; i < n_strategies; i++) {
        if (!w->tries[i])
            continue;
        double rate = w->reward[i] / w->tries[i];
        if (rate > best_rate) {
            best = i;
            best_rate = rate;
        }
    }
    return best;
}
