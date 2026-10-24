#include <math.h>
#include <stdio.h>
#include <string.h>

#include "debounce.h"
#include "machine.h"
#include "memory.h"
#include "walls.h"

static int failures, checks;

#define CHECK(cond)                                                    \
    do {                                                               \
        checks++;                                                      \
        if (!(cond)) {                                                 \
            failures++;                                                \
            fprintf(stderr, "%s:%d: FAIL %s\n", __FILE__, __LINE__, #cond); \
        }                                                              \
    } while (0)

static void test_debounce(void)
{
    debouncer_t d;
    debounce_init(&d, 3);
    CHECK(!debounce_feed(&d, 7));
    CHECK(!debounce_feed(&d, 7));
    CHECK(debounce_feed(&d, 7));
    CHECK(!debounce_feed(&d, 9)); /* a different signal starts over */

    timeout_t t;
    timeout_init(&t, 2);
    CHECK(!timeout_tick(&t));
    CHECK(timeout_tick(&t));
    timeout_reset(&t);
    CHECK(!timeout_tick(&t));
}

static void test_fingerprint(void)
{
    signature_t a, b, c;
    fingerprint("too small by 3198.00", &a);
    fingerprint("too small by 4021.50", &b);
    fingerprint("too small by 2.00", &c);
    CHECK(similarity(&a, &b) == 1.0);  /* same magnitude: same wall */
    CHECK(similarity(&a, &c) < 0.75);  /* different magnitude: new wall */
    CHECK(signature_hash(&a) == signature_hash(&b));

    wallmap_t m;
    wallmap_init(&m, 0.75);
    CHECK(wallmap_match(&m, "syntax error") == -1);
    int w = wallmap_spawn(&m, "syntax error");
    CHECK(wallmap_match(&m, "Syntax Error") == w);
    CHECK(wallmap_match(&m, "division by zero") == -1);
    wallmap_free(&m);
}

static void test_pick_prefers_winner(void)
{
    wall_t w;
    rng_t r;
    memset(&w, 0, sizeof w);
    rng_seed(&r, 1);
    CHECK(wall_best(&w, 3) == -1);
    for (int i = 0; i < 20; i++) {
        wall_record(&w, 0, 0.0);
        wall_record(&w, 1, 1.0);
        wall_record(&w, 2, 0.1);
    }
    CHECK(wall_pick(&w, 3, &r, 1.0) == 1);
    CHECK(wall_best(&w, 3) == 1);
}

static void test_eval(void)
{
    double v;
    CHECK(expr_eval("(2+3)*4-6/3", &v) == 0 && fabs(v - 18) < 1e-9);
    CHECK(expr_eval("2+", &v) == 1);
    CHECK(expr_eval("(2", &v) == 1);
    CHECK(expr_eval("5/0", &v) == 2);
}

static void test_moore_output(void)
{
    /* Output is a function of state alone: same state, same action. */
    state_t s = { S_WALL, 4 };
    CHECK(controller_output(s) == A_ESCAPE);
    signals_t sig = { 0, -1, 0, -1, 1, 0 };
    CHECK(controller_next(s, &sig).kind == S_EXPLORE); /* timeout kicks it */
    signals_t solved = { 1, -1, 0, -1, 0, 1 };
    state_t j = { S_JUDGE, -1 };
    CHECK(controller_next(j, &solved).kind == S_SOLVED); /* solve beats gas */
}

static void test_solves_and_remembers(void)
{
    double target = 137;
    problem_t p = expr_problem(&target);
    wallmap_t walls;
    wallmap_init(&walls, 0.75);
    machine_t m;
    machine_init(&m, &p, &walls, 1, 5000, 3, 25);
    result_t r = machine_run(&m);
    double v;
    CHECK(r.solved);
    CHECK(expr_eval(r.answer, &v) == 0 && fabs(v - target) < 1e-9);
    CHECK(walls.n > 0);

    const char *path = "tests/tmp_memory.txt";
    CHECK(memory_save(path, &p, &walls) == 0);
    wallmap_t back;
    wallmap_init(&back, 0.5);
    CHECK(memory_load(path, &p, &back) == 0);
    CHECK(back.n == walls.n && back.merge_threshold == 0.75);
    for (int i = 0; i < walls.n && i < back.n; i++) {
        CHECK(back.walls[i].hits == walls.walls[i].hits);
        CHECK(similarity(&back.walls[i].sig, &walls.walls[i].sig) == 1.0);
        for (int s = 0; s < p.n_strategies; s++)
            CHECK(back.walls[i].tries[s] == walls.walls[i].tries[s]);
    }
    remove(path);
    wallmap_free(&walls);
    wallmap_free(&back);
}

int main(void)
{
    test_debounce();
    test_fingerprint();
    test_pick_prefers_winner();
    test_eval();
    test_moore_output();
    test_solves_and_remembers();
    printf("%d/%d checks passed\n", checks - failures, checks);
    return failures != 0;
}
