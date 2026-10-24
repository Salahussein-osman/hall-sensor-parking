#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "machine.h"
#include "memory.h"

#define DEBOUNCE 3
#define TIMEOUT 25
#define MERGE 0.75

static void usage(void)
{
    fprintf(stderr,
            "usage: wallbreaker solve <target> [-m memory.txt] [-s seed] [-b budget] [-t]\n"
            "       wallbreaker bench [n]\n");
    exit(2);
}

static void print_walls(const problem_t *p, const wallmap_t *walls)
{
    for (int i = 0; i < walls->n; i++) {
        const wall_t *w = &walls->walls[i];
        int best = wall_best(w, p->n_strategies);
        printf("  W%d '%s' hits=%ld best=%s\n", i, w->example, w->hits,
               best >= 0 ? p->strategy_names[best] : "-");
    }
}

static int solve(int argc, char **argv)
{
    const char *mem = "memory.txt";
    uint64_t seed = 1;
    long budget = 5000;
    int trace = 0;

    if (argc < 1)
        usage();
    double target = atof(argv[0]);
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-t"))
            trace = 1;
        else if (i + 1 >= argc)
            usage();
        else if (!strcmp(argv[i], "-m"))
            mem = argv[++i];
        else if (!strcmp(argv[i], "-s"))
            seed = strtoull(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "-b"))
            budget = atol(argv[++i]);
        else
            usage();
    }

    problem_t p = expr_problem(&target);
    wallmap_t walls;
    wallmap_init(&walls, MERGE);
    if (memory_load(mem, &p, &walls))
        fprintf(stderr, "wallbreaker: ignoring %s (not a %s memory)\n", mem, p.name);

    machine_t m;
    machine_init(&m, &p, &walls, seed, budget, DEBOUNCE, TIMEOUT);
    m.trace = trace;
    result_t r = machine_run(&m);
    if (memory_save(mem, &p, &walls))
        perror("wallbreaker: saving memory");

    printf("%s in %ld steps: %s\n", r.solved ? "SOLVED" : "OUT_OF_GAS", r.steps, r.answer);
    print_walls(&p, &walls);
    wallmap_free(&walls);
    return r.solved ? 0 : 1;
}

static int cmp_long(const void *a, const void *b)
{
    long x = *(const long *)a, y = *(const long *)b;
    return (x > y) - (x < y);
}

/* Same targets twice: once with no memory, once after a warm-up pass. */
static int bench(int argc, char **argv)
{
    int n = argc > 0 ? atoi(argv[0]) : 200;
    if (n <= 0)
        usage();
    double *targets = malloc((size_t)n * sizeof *targets);
    long *steps = malloc((size_t)n * sizeof *steps);
    wallmap_t shared;
    wallmap_init(&shared, MERGE);

    for (int i = 0; i < n; i++) {
        rng_t r;
        rng_seed(&r, (uint64_t)i);
        targets[i] = 50 + rng_below(&r, 4951);
    }
    for (int pass = 0; pass < 3; pass++) {
        int solved = 0;
        for (int i = 0; i < n; i++) {
            problem_t p = expr_problem(&targets[i]);
            wallmap_t fresh;
            wallmap_init(&fresh, MERGE);
            machine_t m;
            /* pass 0: fresh, pass 1: warm-up (unscored), pass 2: with memory */
            machine_init(&m, &p, pass ? &shared : &fresh,
                         (uint64_t)(pass == 1 ? 1000 + i : i), 5000, DEBOUNCE, TIMEOUT);
            result_t r = machine_run(&m);
            solved += r.solved;
            steps[i] = r.steps;
            wallmap_free(&fresh);
        }
        if (pass == 1)
            continue;
        qsort(steps, (size_t)n, sizeof *steps, cmp_long);
        printf("%-12s solved %d/%d  median steps %ld\n",
               pass ? "with memory" : "fresh", solved, n, steps[n / 2]);
    }
    double t = 0;
    problem_t p = expr_problem(&t);
    print_walls(&p, &shared);
    wallmap_free(&shared);
    free(targets);
    free(steps);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2)
        usage();
    if (!strcmp(argv[1], "solve"))
        return solve(argc - 2, argv + 2);
    if (!strcmp(argv[1], "bench"))
        return bench(argc - 2, argv + 2);
    usage();
    return 2;
}
