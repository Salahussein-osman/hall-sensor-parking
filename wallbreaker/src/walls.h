/* Walls: every distinct failure becomes a state with its own stat registers. */
#ifndef WB_WALLS_H
#define WB_WALLS_H

#include <stdint.h>

#include "problem.h"
#include "rng.h"

#define WB_MAX_TOKENS 16
#define WB_TOKEN_LEN 16

/* Token set of an error message. Numbers collapse to a magnitude bucket
 * ("#4" = four digits), so "too small by 3198" and "too small by 2" land
 * in different walls. */
typedef struct {
    int n;
    char tok[WB_MAX_TOKENS][WB_TOKEN_LEN];
} signature_t;

void fingerprint(const char *msg, signature_t *out);
double similarity(const signature_t *a, const signature_t *b);
uint64_t signature_hash(const signature_t *s);

typedef struct {
    signature_t sig;
    char example[WB_MSG_LEN];
    long hits;
    double reward[WB_MAX_STRATEGIES]; /* reward sum per strategy */
    long tries[WB_MAX_STRATEGIES];
} wall_t;

/* The growing set of wall states. Walls are addressed by index (W0, W1..),
 * never by pointer, because the array reallocs as it grows. */
typedef struct {
    wall_t *walls;
    int n, cap;
    double merge_threshold;
} wallmap_t;

void wallmap_init(wallmap_t *m, double merge_threshold);
void wallmap_free(wallmap_t *m);
int wallmap_match(const wallmap_t *m, const char *msg); /* -1 if none */
int wallmap_spawn(wallmap_t *m, const char *msg);

/* UCB1: lean on what worked, still poke at untried escapes. */
int wall_pick(const wall_t *w, int n_strategies, rng_t *rng, double c);
void wall_record(wall_t *w, int strategy, double reward);
int wall_best(const wall_t *w, int n_strategies); /* -1 if nothing tried */

#endif
