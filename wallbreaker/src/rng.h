/* xorshift64: tiny, fast, reproducible. */
#ifndef WB_RNG_H
#define WB_RNG_H

#include <stdint.h>

typedef struct { uint64_t s; } rng_t;

static inline void rng_seed(rng_t *r, uint64_t seed)
{
    r->s = seed * 0x9E3779B97F4A7C15ULL + 1;
    if (!r->s)
        r->s = 1;
}

static inline uint64_t rng_next(rng_t *r)
{
    uint64_t x = r->s;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    return r->s = x;
}

static inline int rng_below(rng_t *r, int n)
{
    return (int)(rng_next(r) % (uint64_t)n);
}

#endif
