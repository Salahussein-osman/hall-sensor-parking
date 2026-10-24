#include "debounce.h"

void debounce_init(debouncer_t *d, int threshold)
{
    d->threshold = threshold;
    debounce_reset(d);
}

int debounce_feed(debouncer_t *d, uint64_t signal)
{
    if (d->count && signal == d->last) {
        d->count++;
    } else {
        d->last = signal;
        d->count = 1;
    }
    return d->count >= d->threshold;
}

void debounce_reset(debouncer_t *d)
{
    d->last = 0;
    d->count = 0;
}

void timeout_init(timeout_t *t, int limit)
{
    t->limit = limit;
    t->count = 0;
}

int timeout_tick(timeout_t *t)
{
    return ++t->count >= t->limit;
}

void timeout_reset(timeout_t *t)
{
    t->count = 0;
}
