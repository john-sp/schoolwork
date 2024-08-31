#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int sys_clone(void)
{
  int fn, stack, arg;
  argint(0, &fn);
  argint(1, &stack);
  argint(2, &arg);
  return clone((void (*)(void*))fn, (void*)stack, (void*)arg);
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  if (n == 0) {
    yield();
    return 0;
  }
  acquire(&tickslock);
  ticks0 = ticks;
  myproc()->sleepticks = n;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  myproc()->sleepticks = -1;
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// P5 - nice system call
int
sys_nice(void) 
{
  int inc;
  if (argint(0, &inc))
  {
    return -1;
  }
  // If thread is already at +19, min value of inc is -39
  // If thread is already at -20, max value of inc is +39
  if (inc < (-39) && inc > 39)
    // Probably don't return failed, just set it to min or max depending on value provided (range clamping)
    // Range Clamping: If an attempt is made to set the nice value beyond its allowed range, 
    // the value is adjusted to the nearest limit within the range (+19 or -20).
    return -1; 
  myproc()->priority += inc;
  if (myproc()->priority < -20)
  {
    myproc()->priority = -20;
  }
  else if (myproc()->priority > 19)
  {
    myproc()->priority = 19;
  }
  
  // cprintf("Changed priority to %d for %s\n", myproc()->priority, myproc()->name);
  myproc()->elevated=myproc()->priority;
  // Return 0 on success
  return 0;
}



int 
sys_macquire(void)
{
  mutex *lock;
  if (argptr(0, (char **)&lock, sizeof(lock)))
  {
    return -1;
  }
  acquire(&lock->lk);
  while (lock->locked)
  {
    if (myproc()->elevated < lock->holder->elevated)
    {
      // cprintf("Changing elevated status from %d to %d on aquire for lock %x\n", myproc()->elevated, lock->holder->elevated, &lock);
      lock->holder->elevated = myproc()->elevated;
    }

    sleep(lock, &lock->lk);
  }

  lock->locked = 1;
  lock->holder = myproc();
  release(&lock->lk);
  return 0;
}
int 
sys_mrelease(void)
{
  mutex *lock;
  if (argptr(0, (char **)&lock, sizeof(lock)))
  {
    return -1;
  }
  acquire(&lock->lk);
  if (myproc()->elevated != myproc()->priority)
  {
    // cprintf("Changing elevated status from %d to %d on aquire\n", myproc()->elevated, myproc()->priority);
    myproc()->elevated = myproc()->priority;
  }
  lock->locked = 0;
  lock->holder = 0;
  wakeup(lock);
  release(&lock->lk);
  return 0;
}