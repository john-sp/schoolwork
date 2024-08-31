typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;

#include "spinlock.h"
typedef struct {
  uint locked;       // Is the lock held?
  struct spinlock lk; // spinlock protecting this sleep lock
  struct proc* holder;
} mutex;