/* Debounce + timeout counters, same idea as a button debouncer circuit. */
#ifndef WB_DEBOUNCE_H
#define WB_DEBOUNCE_H

#include <stdint.h>

/* A signal only counts once it repeats `threshold` times in a row.
 * Filters one-off flukes so they don't spawn junk wall states. */
typedef struct {
    int threshold;
    uint64_t last;
    int count;
} debouncer_t;

void debounce_init(debouncer_t *d, int threshold);
int debounce_feed(debouncer_t *d, uint64_t signal);
void debounce_reset(debouncer_t *d);

/* Counts ticks spent stuck; fires at `limit` so we never wait forever. */
typedef struct {
    int limit;
    int count;
} timeout_t;

void timeout_init(timeout_t *t, int limit);
int timeout_tick(timeout_t *t);
void timeout_reset(timeout_t *t);

#endif
