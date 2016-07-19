#pragma once

#include <stdint.h>

typedef struct Progress *Progress;

/* Starts a new progress display. */
extern Progress progress_new(int64_t total);

/* Updates the progress. */
extern void progress_update(Progress p, int64_t value);
extern void progress_update_total(Progress p, int64_t value, int64_t total);

/* Finishes the progress and destroys all its ressources. */
extern void progress_finish(Progress p, int64_t value);
