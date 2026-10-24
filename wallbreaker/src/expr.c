#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "problem.h"

#define EXPR_MAX_LEN 60

static const char *const NAMES[] = {
    "add", "sub", "mul", "div", "tweak", "chop", "scramble",
};
enum { ADD, SUB, MUL, DIV, TWEAK, CHOP, SCRAMBLE, N_STRATEGIES };

static const char CHARS[] = "0123456789+-*/";

/* Recursive descent: expr = term (+|- term)*, term = factor (*|/ factor)*,
 * factor = number | ( expr ). */
typedef struct {
    const char *s;
    int syntax;
    int divzero;
} parser_t;

static double p_expr(parser_t *p);

static double p_factor(parser_t *p)
{
    if (*p->s == '(') {
        p->s++;
        double v = p_expr(p);
        if (*p->s != ')') {
            p->syntax = 1;
            return 0;
        }
        p->s++;
        return v;
    }
    if (!isdigit((unsigned char)*p->s)) {
        p->syntax = 1;
        return 0;
    }
    double v = 0;
    while (isdigit((unsigned char)*p->s))
        v = v * 10 + (*p->s++ - '0');
    return v;
}

static double p_term(parser_t *p)
{
    double v = p_factor(p);
    while (!p->syntax && (*p->s == '*' || *p->s == '/')) {
        char op = *p->s++;
        double r = p_factor(p);
        if (op == '*')
            v *= r;
        else if (r == 0)
            p->divzero = 1;
        else
            v /= r;
    }
    return v;
}

static double p_expr(parser_t *p)
{
    double v = p_term(p);
    while (!p->syntax && (*p->s == '+' || *p->s == '-')) {
        char op = *p->s++;
        double r = p_term(p);
        v = op == '+' ? v + r : v - r;
    }
    return v;
}

int expr_eval(const char *s, double *out)
{
    parser_t p = { s, 0, 0 };
    *out = p_expr(&p);
    if (p.syntax || *p.s)
        return 1;
    return p.divzero ? 2 : 0;
}

static char digit(rng_t *rng)
{
    return (char)('1' + rng_below(rng, 9));
}

static void seed(const problem_t *p, char *cand, rng_t *rng)
{
    (void)p;
    cand[0] = digit(rng);
    cand[1] = '\0';
}

static verdict_t judge(const problem_t *p, const char *cand)
{
    verdict_t v = { 0, "", INFINITY };
    double value, target = *(const double *)p->ctx;

    if (strlen(cand) > EXPR_MAX_LEN) {
        snprintf(v.error, sizeof v.error, "expression too long");
        return v;
    }
    switch (expr_eval(cand, &value)) {
    case 1:
        snprintf(v.error, sizeof v.error, "syntax error");
        return v;
    case 2:
        snprintf(v.error, sizeof v.error, "division by zero");
        return v;
    }
    double diff = value - target;
    if (fabs(diff) < 1e-9) {
        v.ok = 1;
        v.score = 0;
        return v;
    }
    v.score = fabs(diff);
    snprintf(v.error, sizeof v.error, "too %s by %.2f",
             diff < 0 ? "small" : "big", v.score);
    return v;
}

static void apply(const problem_t *p, int strategy, char *cand, rng_t *rng)
{
    (void)p;
    size_t len = strlen(cand);
    char d = digit(rng);

    switch (strategy) {
    case ADD:
    case SUB:
        if (len + 3 > WB_CAND_MAX)
            return;
        cand[len] = strategy == ADD ? '+' : '-';
        cand[len + 1] = d;
        cand[len + 2] = '\0';
        break;
    case MUL:
    case DIV:
        if (len + 5 > WB_CAND_MAX)
            return;
        memmove(cand + 1, cand, len);
        cand[0] = '(';
        cand[len + 1] = ')';
        cand[len + 2] = strategy == MUL ? '*' : '/';
        cand[len + 3] = d;
        cand[len + 4] = '\0';
        break;
    case TWEAK: {
        int spots[WB_CAND_MAX], n = 0;
        for (size_t i = 0; i < len; i++)
            if (isdigit((unsigned char)cand[i]))
                spots[n++] = (int)i;
        if (n)
            cand[spots[rng_below(rng, n)]] = d;
        break;
    }
    case CHOP:
        if (len > 1)
            cand[len - 1] = '\0';
        else
            cand[0] = d;
        break;
    case SCRAMBLE:
        cand[rng_below(rng, (int)len)] = CHARS[rng_below(rng, sizeof CHARS - 1)];
        break;
    }
}

problem_t expr_problem(const double *target)
{
    problem_t p = {
        .name = "expression",
        .n_strategies = N_STRATEGIES,
        .strategy_names = NAMES,
        .seed = seed,
        .judge = judge,
        .apply = apply,
        .ctx = target,
    };
    return p;
}
