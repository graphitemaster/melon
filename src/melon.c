#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <limits.h>

#include "private.h"

struct melon g_melon;

int melon_init(uint16_t nb_threads)
{
  unsigned i = 0;

  /* default values */
  g_melon.stop = 0;

  /* init the spin mutex */
  if (!pthread_mutex_init(&g_melon.mutex, NULL))
    return -1;

  /* get the number of threads in the thread pull */
  if (nb_threads == 0)
    nb_threads = sysconf(_SC_NPROCESSORS_ONLN);
  g_melon.threads_nb = nb_threads > 0 ? nb_threads : 1;

  /* init the ready queue */
  g_melon.ready = 0;

  /* get the limit of opened file descriptors */
  int rlimit_nofile = _POSIX_OPEN_MAX;
  struct rlimit rl;
  if (!getrlimit(RLIMIT_NOFILE, &rl))
    rlimit_nofile = rl.rlim_max;

  /* allocate io_blocked */
  g_melon.io_blocked = calloc(1, sizeof (*g_melon.io_blocked) * rlimit_nofile);

  /* initialize io manager */
  if (melon_io_manager_init())
    goto failure_calloc;

  /* initialize timer manager */
  if (melon_timer_manager_init())
    goto failure_io_manager;

  /* allocates thread pool */
  g_melon.threads = malloc(sizeof (*g_melon.threads) * g_melon.threads_nb);
  for (i = 0; i < g_melon.threads_nb; ++i)
    if (pthread_create(g_melon.threads + i, 0, melon_sched_run, 0))
      goto failure_pthread;

  /* succeed */
  return 0;

  failure_pthread:
  g_melon.stop = 1;
  /* join thread pool and free threads */
  for (; i > 0; --i)
    pthread_join(g_melon.threads[i - 1], NULL);
  free(g_melon.threads);
  g_melon.threads = NULL;

  failure_io_manager:
  /* deinit io manager */
  melon_io_manager_deinit();

  failure_calloc:
  /* free io_blocked */
  free(g_melon.io_blocked);
  g_melon.io_blocked = NULL;
  return -1;
}

void melon_deinit()
{
  /* io_manager */
  melon_io_manager_deinit();

  /* timer_manager */
  melon_timer_manager_deinit();

  /* thread pool */
  for (unsigned i = 0; i < g_melon.threads_nb; ++i)
    pthread_join(g_melon.threads[i], 0);
  free(g_melon.threads);
  g_melon.threads = NULL;

  /* io_blocked */
  free(g_melon.io_blocked);
  g_melon.io_blocked = NULL;
}