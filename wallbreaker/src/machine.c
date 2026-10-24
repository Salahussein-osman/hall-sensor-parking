#include <math.h>
#include <stdio.h>
#include <string.h>

#include "machine.h"

/* ---- controller (Moore) ------------------------------------------------ */

action_t controller_output(state_t s)
{
    switch (s.kind) {
    case S_SEED:     return A_SEED;
    case S_JUDGE:    return A_JUDGE;
    case S_DEBOUNCE: return A_DEBOUNCE;
    case S_SPAWN:    return A_SPAWN;
    case S_EXPLORE:  return A_EXPLORE;
    case S_WALL:     return A_ESCAPE;
    default:         return A_HALT;
    }
}

static state_t st(state_kind_t kind, int wall)
{
    state_t s = { kind, wall };
    return s;
}

state_t controller_next(state_t s, const signals_t *sig)
{
    state_t next = s;

    switch (s.kind) {
    case S_SEED:
        next = st(S_JUDGE, -1);
        break;
    case S_JUDGE:
        if (sig->ok)
            next = st(S_SOLVED, -1);
        else if (sig->known_wall >= 0)
            next = st(S_WALL, sig->known_wall);
        else
            next = st(S_DEBOUNCE, -1);
        break;
    case S_DEBOUNCE:
        next = st(sig->confirmed ? S_SPAWN : S_EXPLORE, -1);
        break;
    case S_SPAWN:
        next = st(S_WALL, sig->wall);
        break;
    case S_EXPLORE:
        next = st(S_JUDGE, -1);
        break;
    case S_WALL:
        next = sig->timed_out ? st(S_EXPLORE, -1) : st(S_JUDGE, -1);
        break;
    default:
        break;
    }
    if (sig->out_of_gas && next.kind != S_SOLVED)
        next = st(S_OUT_OF_GAS, -1);
    return next;
}

const char *state_name(state_t s, char *buf, size_t n)
{
    static const char *const names[] = {
        "SEED", "JUDGE", "DEBOUNCE", "SPAWN", "EXPLORE", "WALL",
        "SOLVED", "OUT_OF_GAS",
    };
    if (s.kind == S_WALL)
        snprintf(buf, n, "WALL:W%d", s.wall);
    else
        snprintf(buf, n, "%s", names[s.kind]);
    return buf;
}

const char *action_name(action_t a)
{
    static const char *const names[] = {
        "seed", "judge", "debounce", "spawn", "explore", "escape", "halt",
    };
    return names[a];
}

/* ---- datapath ---------------------------------------------------------- */

static void dp_move(datapath_t *dp, int strategy)
{
    memcpy(dp->prev_cand, dp->cand, sizeof dp->cand);
    dp->prev_verdict = dp->verdict;
    dp->has_prev = 1;
    dp->problem->apply(dp->problem, strategy, dp->cand, &dp->rng);
}

/* 0..1: how much of the gap a move closed. */
static double reward(const verdict_t *v, double old, int improved, int left_wall)
{
    if (v->ok)
        return 1.0;
    if (improved)
        return isinf(old) ? 1.0 : (old - v->score) / old;
    return left_wall ? 0.5 : 0.0;
}

static void dp_seed(datapath_t *dp)
{
    dp->problem->seed(dp->problem, dp->cand, &dp->rng);
    dp->has_prev = 0;
    debounce_reset(&dp->debouncer);
}

static void dp_judge(datapath_t *dp, signals_t *sig)
{
    verdict_t v = dp->problem->judge(dp->problem, dp->cand);
    double old = dp->has_prev ? dp->prev_verdict.score : INFINITY;
    int improved = v.score < old;

    /* Hill-climb: a move that made things worse gets undone. */
    if (v.score > old) {
        memcpy(dp->cand, dp->prev_cand, sizeof dp->cand);
        v = dp->prev_verdict;
    }
    dp->verdict = v;

    int wall = v.ok ? -1 : wallmap_match(dp->walls, v.error);
    if (dp->pending_wall >= 0) {
        wall_record(&dp->walls->walls[dp->pending_wall], dp->pending_strategy,
                    reward(&v, old, improved, wall != dp->pending_wall));
        dp->pending_wall = -1;
    }
    /* Only being stuck counts toward the timeout; progress resets it. */
    if (improved || wall != dp->current_wall)
        timeout_reset(&dp->timeout);
    dp->current_wall = wall;
    if (wall >= 0)
        dp->walls->walls[wall].hits++;
    sig->ok = v.ok;
    sig->known_wall = wall;
}

static void dp_debounce(datapath_t *dp, signals_t *sig)
{
    signature_t s;
    fingerprint(dp->verdict.error, &s);
    sig->confirmed = debounce_feed(&dp->debouncer, signature_hash(&s));
}

static void dp_spawn(datapath_t *dp, signals_t *sig)
{
    int wall = wallmap_spawn(dp->walls, dp->verdict.error);
    dp->walls->walls[wall].hits++;
    dp->current_wall = wall;
    debounce_reset(&dp->debouncer);
    timeout_reset(&dp->timeout);
    sig->wall = wall;
}

static void dp_explore(datapath_t *dp)
{
    dp_move(dp, rng_below(&dp->rng, dp->problem->n_strategies));
}

static void dp_escape(datapath_t *dp, int wall, signals_t *sig)
{
    if (timeout_tick(&dp->timeout)) {
        /* Stuck too long: let EXPLORE kick it loose. Progress is kept. */
        timeout_reset(&dp->timeout);
        sig->timed_out = 1;
        return;
    }
    int strategy = wall_pick(&dp->walls->walls[wall], dp->problem->n_strategies,
                             &dp->rng, 1.0);
    dp_move(dp, strategy);
    dp->pending_wall = wall;
    dp->pending_strategy = strategy;
}

/* ---- machine ----------------------------------------------------------- */

void machine_init(machine_t *m, const problem_t *p, wallmap_t *walls,
                  uint64_t seed, long budget, int debounce, int timeout)
{
    memset(m, 0, sizeof *m);
    m->state = st(S_SEED, -1);
    m->budget = budget;
    m->dp.problem = p;
    m->dp.walls = walls;
    m->dp.pending_wall = -1;
    m->dp.current_wall = -1;
    rng_seed(&m->dp.rng, seed);
    debounce_init(&m->dp.debouncer, debounce);
    timeout_init(&m->dp.timeout, timeout);
}

int machine_step(machine_t *m)
{
    action_t a = controller_output(m->state);
    signals_t sig = { 0, -1, 0, -1, 0, 0 };

    if (a == A_HALT)
        return 0;
    switch (a) {
    case A_SEED:     dp_seed(&m->dp); break;
    case A_JUDGE:    dp_judge(&m->dp, &sig); break;
    case A_DEBOUNCE: dp_debounce(&m->dp, &sig); break;
    case A_SPAWN:    dp_spawn(&m->dp, &sig); break;
    case A_EXPLORE:  dp_explore(&m->dp); break;
    case A_ESCAPE:   dp_escape(&m->dp, m->state.wall, &sig); break;
    case A_HALT:     break;
    }
    m->steps++;
    sig.out_of_gas = m->steps >= m->budget;

    state_t next = controller_next(m->state, &sig);
    if (m->trace) {
        char a_buf[32], b_buf[32];
        printf("%12s --%s--> %s\n", state_name(m->state, a_buf, sizeof a_buf),
               action_name(a), state_name(next, b_buf, sizeof b_buf));
    }
    m->state = next;
    return 1;
}

result_t machine_run(machine_t *m)
{
    while (machine_step(m))
        ;
    result_t r = { m->state.kind == S_SOLVED, m->steps, m->dp.cand };
    return r;
}
