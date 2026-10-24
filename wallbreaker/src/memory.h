/* Persist the wall map between runs so the stats keep growing.
 *
 * Plain text, one wall per block:
 *   wallbreaker 1
 *   problem expression
 *   merge 0.75
 *   wall <hits> <ntokens> <tok>...
 *   example <message>
 *   stat <strategy> <reward-sum> <tries>
 *   end
 */
#ifndef WB_MEMORY_H
#define WB_MEMORY_H

#include "problem.h"
#include "walls.h"

/* 0 = loaded (or no file yet), -1 = unreadable / for another problem. */
int memory_load(const char *path, const problem_t *p, wallmap_t *m);
int memory_save(const char *path, const problem_t *p, const wallmap_t *m);

#endif
