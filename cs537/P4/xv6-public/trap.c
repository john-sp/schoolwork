#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "traps.h"
#include "wmap.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;
  case T_PGFLT: // T_PGFLT = 14
    uint accessAddr = rcr2();
    if (accessAddr >= WMAP_BASE && accessAddr < KERNBASE) {
      // cprintf("Page fault on access of %x\n", accessAddr);
      struct proc *curproc = myproc();
      for (int i = 0; i < MAX_VMAPS; i++)
      {
        if (curproc->vmas[i].mapped == 1)
        {
          // cprintf("Checking vmap %d with lower of %x and upper of %x\n", i, curproc->vmas[i].addr, curproc->vmas[i].addr + curproc->vmas[i].length);
          if (curproc->vmas[i].addr <= accessAddr && curproc->vmas[i].addr + curproc->vmas[i].length >= accessAddr)
          {
            // cprintf("flags for 0x%x: %x\n",accessAddr,curproc->vmas[i].flags & MAP_ANONYMOUS);
            if (curproc->vmas[i].flags & MAP_ANONYMOUS)
            {
              // cprintf("attempting lazy alloc for page starting with %x to %x\n", accessAddr, PGROUNDDOWN(accessAddr)); //  + (uint)i * PGSIZE)
              char *mem = kalloc(); // Returns kernal allowed mem addr
              if (!mem)
              {
                cprintf("OOM\n");
                myproc()->killed = 1;
                return;
              }
              memset(mem, 0, PGSIZE);

              mappages(curproc->pgdir, (void *)(PGROUNDDOWN(accessAddr)), (uint)PGSIZE, V2P(mem), PTE_W | PTE_U);
              curproc->vmas[i].num_loaded += 1;
              return;
            }
            else
            { 
              // cprintf("Reading from file\n\n");                      // File mapped page
              char *mem = kalloc(); // Returns kernal allowed mem addr
              if (!mem)
              {
                cprintf("OOM\n");
                myproc()->killed = 1;
                return;
              }
              memset(mem, 0, PGSIZE);
              // TODO: Copy data from file
              // USEFUL things: 
              // curproc->vmas[i].f: file pointer we are copying from
              // readi: copy data from inode to char*, potentially can do the data copy
              // readi(struct inode *ip, char *dst, uint off, uint n): note Caller must hold ip->lock.
              // memmove(void *dst, const void *src, uint n) may help
              struct file* f = curproc->vmas[i].f;
              mappages(curproc->pgdir, (void *)(PGROUNDDOWN(accessAddr + (uint)i * PGSIZE)), (uint)PGSIZE, V2P(mem), PTE_W | PTE_U);

              ilock(f->ip);
              readi(f->ip, mem, accessAddr-curproc->vmas[i].addr, PGSIZE);
              iunlock(f->ip);
              // fileread(file, (void *)curproc->vmas[i].addr, PGSIZE);
              curproc->vmas[i].num_loaded += 1;
              return;
            }
          }
        }
      }
      cprintf("Segmentation Fault on access of %x\n", accessAddr);
      myproc()->killed = 1;
      break;
      // kill the process
    
  }


  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
