#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "../src/melon.h"

static void timer(void * dummy)
{
  (void)dummy;
  for (int i = 0; i < 20; ++i)
    melon_usleep((rand() % 5) * 10000);
  melon_sleep(2);
}

int main(void)
{
  if (melon_init(0))
    return 1;

  for (int i = 1; i <= 100; ++i)
  {
    if (melon_fiber_start(timer, NULL + i))
      break;
  }

  printf("waiting\n");
  melon_wait();
  printf("wait finished\n");
  melon_deinit();
  return 0;
}