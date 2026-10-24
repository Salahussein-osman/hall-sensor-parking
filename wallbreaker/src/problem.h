/* A problem = a seed, a judge, and a bag of moves (strategies).
 *
 * judge() returns a verdict: `error` is the wall message, `score` is the
 * distance from solved (lower is better, INFINITY when the candidate is
 * broken). */
#ifndef WB_PROBLEM_H
#define WB_PROBLEM_H

#include "rng.h"

#define WB_CAND_MAX 128
#define WB_MSG_LEN 64
#define WB_MAX_STRATEGIES 16

typedef struct {
    int ok;
    char error[WB_MSG_LEN];
    double score;
} verdict_t;

typedef struct problem {
    const char *name;
    int n_strategies;
    const char *const *strategy_names;
    void (*seed)(const struct problem *p, char *cand, rng_t *rng);
    verdict_t (*judge)(const struct problem *p, const char *cand);
    void (*apply)(const struct problem *p, int strategy, char *cand, rng_t *rng);
    const void *ctx;
} problem_t;

/* Toy problem: build an arithmetic expression equal to *target. */
problem_t expr_problem(const double *target);
/* 0 = ok, 1 = syntax error, 2 = division by zero. */
int expr_eval(const char *s, double *out);

#endif
