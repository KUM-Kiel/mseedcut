#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include "progress.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

struct Progress {
  pthread_mutex_t lock[1];
  pthread_cond_t condition[1];
  pthread_t thread;
  int64_t value;
  int64_t total;
  int done;
};

#ifdef _POSIX_MONOTONIC_CLOCK
#define CLOCK CLOCK_MONOTONIC
#else
#define CLOCK CLOCK_REALTIME
#endif

#define SEC (1000000000ll)
#define SLEEP (SEC / 5)

static const char *si_prefix[] = {" ", "k", "M", "G", "T", "P", "E", "Z", "Y"};
static size_t format_size(char *s, size_t slen, double size)
{
  double x = size > 0 ? size : 0;
  int e = 0;
  while (x >= 1000 && e <= 8) {
    x /= 1000;
    e += 1;
  }
  if (x < 10) {
    return snprintf(s, slen, "%3.1f %sB", x, si_prefix[e]);
  } else if (x < 1000) {
    return snprintf(s, slen, "%3.0f %sB", x, si_prefix[e]);
  } else {
    return snprintf(s, slen, "---  B");
  }
}

static size_t format_time(char *s, size_t slen, double sec)
{
  if (sec > 3600) {
    return snprintf(s, slen, "%2d:%02d:%02d",
      (int) ((int64_t) sec / 3600),
      (int) (((int64_t) sec % 3600) / 60),
      (int) ((int64_t) sec % 60));
  } else {
    return snprintf(s, slen, "   %2d:%02d",
      (int) ((int64_t) sec / 60),
      (int) ((int64_t) sec % 60));
  }
}

static void *progress_thread(void *x)
{
  Progress p = x;
  struct timespec start[1], now[1];
  double speed = 0, tdiff = 0;
  char s[4][64];

  clock_gettime(CLOCK, start);

  pthread_mutex_lock(p->lock);
  while (1) {
    /* Get the time. */
    clock_gettime(CLOCK, now);
    tdiff = (now->tv_sec - start->tv_sec) +
            (now->tv_nsec - start->tv_nsec) / (double) SEC;
    /* Print the progress. */
    if (tdiff > 0) {
      speed = p->value / tdiff;
    }
    format_size(s[0], sizeof(s[0]), p->value);
    format_size(s[1], sizeof(s[1]), speed);
    if (speed > 0 && p->total && !p->done) {
      format_time(s[2], sizeof(s[2]), (p->total - p->value) / speed);
    } else {
      format_time(s[2], sizeof(s[2]), tdiff);
    }
    if (p->total) {
      fprintf(stderr, "\033[K%3d%% %s | %s/s %s%s",
        (int) (100 * p->value / p->total),
        s[0], s[1], s[2],
        p->done ? "\n" : "\r");
    } else {
      fprintf(stderr, "\033[K...%% %s | %s/s %s%s",
        s[0], s[1], s[2],
        p->done ? "\n" : "\r");
    }
    fflush(stderr);
    if (p->done) break;
    /* Wait 100ms */
    now->tv_nsec += SLEEP;
    while (now->tv_nsec >= SEC) {
      now->tv_nsec -= SEC;
      now->tv_sec += 1;
    }
    pthread_cond_timedwait(p->condition, p->lock, now);
  }
  pthread_mutex_unlock(p->lock);
  return 0;
}

extern Progress progress_new(int64_t total)
{
  Progress p = malloc(sizeof(struct Progress));
  pthread_condattr_t a[1];

  if (!p) return 0;

  /* XXX: Proper destruction on fail. */
  pthread_condattr_init(a);
  pthread_condattr_setclock(a, CLOCK);

  pthread_mutex_init(p->lock, 0);
  pthread_cond_init(p->condition, a);
  p->value = 0;
  p->total = total > 0 ? total : 0;
  p->done = 0;

  if (pthread_create(&p->thread, 0, progress_thread, p)) goto fail;

  return p;
fail:
  free(p);
  return 0;
}

extern void progress_update(Progress p, int64_t value)
{
  if (!p) return;

  pthread_mutex_lock(p->lock);
  value = value > 0 ? value : 0;
  if (p->total)
    value = value < p->total ? value : p->total;
  p->value = value;
  pthread_mutex_unlock(p->lock);
}

extern void progress_update_total(Progress p, int64_t value, int64_t total)
{
  if (!p) return;

  pthread_mutex_lock(p->lock);
  value = value > 0 ? value : 0;
  total = total > 0 ? total : 0;
  if (total)
    value = value < total ? value : total;
  p->value = value;
  p->total = total;
  pthread_mutex_unlock(p->lock);
}

extern void progress_finish(Progress p, int64_t value)
{
  if (!p) return;

  pthread_mutex_lock(p->lock);
  value = value > 0 ? value : 0;
  if (p->total)
    value = value < p->total ? value : p->total;
  p->value = value;
  p->done = 1;
  pthread_cond_broadcast(p->condition);
  pthread_mutex_unlock(p->lock);

  pthread_join(p->thread, 0);
  pthread_mutex_destroy(p->lock);
  pthread_cond_destroy(p->condition);
  free(p);
}
