#include "types.h"
#include "user.h"

mutex m;
volatile int flag = 0;

void fn1(void* arg) {
  macquire(&m);
  nice(10);
  __sync_synchronize();
  flag = 1;
  printf(1, "fn1: addr %x\n", &m);
  __sync_synchronize();
  for (int i = 0; i < 10000; i++) {
    if (i % 1000 == 0) {
      printf(1, "#fn1");
      // sleep(0) is equivalent to yield(). This gives the scheduler more chances to do scheduling
      sleep(0);
    }
  }
  mrelease(&m);
  sleep(0);
  for (int i = 0; i < 10000; i++) {
    if (i % 1000 == 0) {
      printf(1, "#fn1");

      // sleep(0) is equivalent to yield(). This gives the scheduler more chances to do scheduling
      sleep(0);
    }
  }

  exit();
}

void fn2(void* arg) {
  while(!flag) sleep(1);
  __sync_synchronize();
  sleep(0);
  nice(-10);
  printf(1, "fn2: addr %x\n", &m);
  macquire(&m);
  for (int i = 0; i < 10000; i++) {
    if (i % 1000 == 0) {
      printf(1, "#fn2");
      sleep(0);
    }
  }
  mrelease(&m);
  
  exit();
}

int main() {
  minit(&m);
  char* stack1 = (char*)malloc(4096);
  char* stack2 = (char*)malloc(4096);

  clone(fn1, stack1 + 4096, 0);
  clone(fn2, stack2 + 4096, 0);
  sleep(0);
  // sleep(0);
  printf(1, "main: addr %x\n", &m);
  while(!flag);
  __sync_synchronize();

  for (int i = 0; i < 10000; i++) {
    if (i % 1000 == 0) {
      printf(1, "#main");
      sleep(0);
    }
  }

  wait();
  wait();
  exit();
}
