/* The FSMD: a Moore controller driving a datapath.
 *
 * Controller: output depends only on the current state. A few fixed states
 * plus one dynamic state per wall (S_WALL + wall index), so the state set
 * grows every time the machine hits something new.
 *
 * Datapath: holds the candidate, the wall map (stat registers), the
 * debouncer and the timeout counter. Each output action runs here and
 * returns status signals the controller uses to pick the next state. */
#ifndef WB_MACHINE_H
#define WB_MACHINE_H

#include <stddef.h>

#include "debounce.h"
#include "problem.h"
#include "walls.h"

typedef enum {
    S_SEED,      /* start from a fresh candidate */
    S_JUDGE,     /* run the judge, look the failure up in the wall map */
    S_DEBOUNCE,  /* unknown failure: has it repeated enough to be real? */
    S_SPAWN,     /* confirmed: grow a new wall state */
    S_EXPLORE,   /* unknown territory (or timed out): random move */
    S_WALL,      /* known wall: escape with the best move on record */
    S_SOLVED,
    S_OUT_OF_GAS,
} state_kind_t;

typedef struct {
    state_kind_t kind;
    int wall; /* only meaningful for S_WALL */
} state_t;

typedef enum {
    A_SEED, A_JUDGE, A_DEBOUNCE, A_SPAWN, A_EXPLORE, A_ESCAPE, A_HALT,
} action_t;

typedef struct {
    int ok;
    int known_wall; /* -1 if the failure matched no wall */
    int confirmed;
    int wall;
    int timed_out;
    int out_of_gas;
} signals_t;

action_t controller_output(state_t s);
state_t controller_next(state_t s, const signals_t *sig);
const char *state_name(state_t s, char *buf, size_t n);
const char *action_name(action_t a);

typedef struct {
    const problem_t *problem;
    wallmap_t *walls;
    rng_t rng;
    debouncer_t debouncer;
    timeout_t timeout;
    char cand[WB_CAND_MAX];
    verdict_t verdict;
    char prev_cand[WB_CAND_MAX]; /* candidate before the last move */
    verdict_t prev_verdict;
    int has_prev;
    int pending_wall, pending_strategy; /* escape waiting on a judge result */
    int current_wall;
} datapath_t;

typedef struct {
    state_t state;
    datapath_t dp;
    long steps, budget;
    int trace;
} machine_t;

typedef struct {
    int solved;
    long steps;
    const char *answer;
} result_t;

void machine_init(machine_t *m, const problem_t *p, wallmap_t *walls,
                  uint64_t seed, long budget, int debounce, int timeout);
int machine_step(machine_t *m); /* 0 once halted */
result_t machine_run(machine_t *m);

#endif
