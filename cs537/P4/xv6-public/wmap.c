#include "types.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"
#include "defs.h"
#include "memlayout.h"
#include "wmap.h"

void copy_vma_struct(struct vma *vma1, struct vma *vma2)
{
    vma1->addr = vma2->addr;
    vma1->end = vma2->end;
    vma1->length = vma2->length;
    vma1->flags = vma2->flags;
    vma1->f = vma2->f;
    vma1->mapped = vma2->mapped;
    vma1->ref = vma2->ref;
}

int right_shift_vmas(struct proc *p, int i, uint wmapaddr, int length)
{
    // cprintf("1) Location: %d addr: 0x%x\n\n", i + 1, wmapaddr);
    int j = p->total_vmaps;
    while (j > i + 1)
    {
        copy_vma_struct(&p->vmas[j], &p->vmas[j - 1]);
        j--;
    }
    // cprintf("2) Location: %d addr: 0x%x, len: %d\n\n", i + 1, wmapaddr, length);
    p->vmas[i + 1].addr = wmapaddr;
    p->vmas[i + 1].length = length;
    return i + 1; // Return the index of mapping
}

int find_vmap_addr(struct proc *p, int size)
{
    if (p->total_vmaps == 0)
    {
        if (PGROUNDUP(WMAP_BASE + size) >= KERNBASE)
        {
            // Address Exceeds KERNBASE
            return -1;
        }
        p->vmas[0].addr = PGROUNDUP(WMAP_BASE);
        p->vmas[0].length = size;
        // cprintf("Using location 0x%x with size %d\n", PGROUNDUP(WMAP_BASE), p->vmas[0].length);
        return 0; // Return the index in wmap vmas array
    }
    int i = 0;
    uint addr;
    // If mapping is possible between WMAP_BASE & first mapping
    if (p->vmas[0].addr - WMAP_BASE > size)
    {
        addr = WMAP_BASE;
        return right_shift_vmas(p, -1, addr, size);
    }
    // Find the map address
    while (i < p->total_vmaps && p->vmas[i + 1].addr != 0)
    {
        uint start_addr = PGROUNDUP(p->vmas[i].addr + p->vmas[i].length);
        uint end_addr = PGROUNDUP(p->vmas[i + 1].addr);
        if (end_addr - start_addr > size - 1)
        {
            break;
        }
        i += 1;
    }
    addr = PGROUNDUP(p->vmas[i].addr + p->vmas[i].length);
    if (addr + size > KERNBASE)
    {
        return -1;
    }
    // cprintf("Using address 0x%x\n", addr);
    // Right shift the mappings to arrange in increasing order
    return right_shift_vmas(p, i, addr, size);
}

uint mywmap(uint addr, int length, int flags, struct file *f)
{
    int private = flags & MAP_PRIVATE;
    int shared = flags & MAP_SHARED;
    // int anon = flags & MAP_ANONYMOUS;

    int fixed = flags & MAP_FIXED;

    if (private != 0 && shared != 0)
    {
        cprintf("Must be only private or shared\n");
        return FAILED;
    }
    if (private == 0 && shared == 0)
    {
        cprintf("Must be private or shared\n");
        return FAILED;
    }

    struct proc *curproc = myproc();

    struct vma *vma = 0; // Initialize vma pointer to NULL
    int i;

    // Find an empty slot in the vmas array
    for (i = 0; i < MAX_VMAPS; i++)
    {
        if (curproc->vmas[i].mapped == 0)
        {
            vma = &curproc->vmas[i];
            break;
        }
    }

    // No empty slot found
    if (vma == 0)
    {
        cprintf("VMA Full\n");
        return FAILED; // Return FAILED if no empty slot is found
    }
    if (fixed)
    {
        if (addr % PGSIZE != 0)
        {
            cprintf("memory location %d is not page aligned\n", addr);
            return FAILED;
        }
        if (addr < WMAP_BASE || addr + length > KERNBASE)
        {
            // cprintf("location out side of bounds\n");
            return FAILED;
        }

        for (int j = 0; j < MAX_VMAPS; j++)
        {
            if (curproc->vmas[j].mapped)
            {
                if ((addr >= curproc->vmas[j].addr && addr < curproc->vmas[j].end) ||
                    (addr + length > curproc->vmas[j].addr && addr + length <= curproc->vmas[j].end))
                {
                    // New mapping overlaps with existing mapping
                    cprintf("Location already used\n");
                    return FAILED;
                }
            }
        }
        // Insert the new VMA into the sorted array
        int j;
        for (j = curproc->total_vmaps - 1; j >= 0; j--)
        {
            if (curproc->vmas[j].addr > addr)
            {
                // cprintf("shift! %x vs %x\n", curproc->vmas[j].addr, addr);
                copy_vma_struct(&(curproc->vmas[j + 1]), &(curproc->vmas[j]));
            }
            else
            {
                break;
            }
        }

        curproc->vmas[j + 1].addr = addr;
        curproc->vmas[j + 1].end = (addr + length);
        curproc->vmas[j + 1].length = length;
        curproc->vmas[j + 1].flags = flags;
        curproc->vmas[j + 1].f = f;
        curproc->vmas[j + 1].mapped = 1;
        curproc->vmas[j + 1].num_loaded = 0;
        curproc->vmas[j + 1].ref = 0;
        curproc->total_vmaps += 1;
        return addr;
    }
    else // Not a fixed mapping
    {

        // cprintf("looking for location for length %d\n", length);
        int position = find_vmap_addr(curproc, length);
        curproc->vmas[position].end = (curproc->vmas[position].addr + length);
        curproc->vmas[position].length = length;
        curproc->vmas[position].flags = flags;
        curproc->vmas[position].f = f;
        curproc->vmas[position].mapped = 1;
        curproc->vmas[position].num_loaded = 0;
        curproc->vmas[position].ref = 0;
        curproc->total_vmaps += 1;
        // cprintf("location found in position %d\n", position);
        // for (int j = 0; j < MAX_VMAPS; j++)
        // {
        //     cprintf("%d(0x%x, pages: %x) ", j, curproc->vmas[j].addr, (curproc->vmas[j].length - 1) / PGSIZE);
        // }
        // cprintf("\n");

        return curproc->vmas[position].addr;
    }
    cprintf("How did we get here?\n");
    return FAILED;
}

int mywunmap(uint addr)
{
    struct proc *curproc = myproc();
    struct vma *mem = 0;
    int i = 0;
    for (i = 0; i < MAX_VMAPS; i++)
    {
        if (curproc->vmas[i].mapped == 1 && curproc->vmas[i].addr == addr)
        {
            mem = &curproc->vmas[i];
            break;
        }
    }

    if (mem == 0 || mem->mapped == 0)
    {
        // location in VMA not found or not allocated
        return FAILED;
    }

    int nuvmas = (mem->length - 1) / PGSIZE + 1;
    for (int j = 0; j < nuvmas; j++)
    {
        pte_t *pte = walkpgdir(curproc->pgdir, (void *)addr + j * PGSIZE, 0);
        uint physical_address = PTE_ADDR(*pte);
        // cprintf("Unmapping virt: %x, phys: %x\n", addr + j * PGSIZE, physical_address);
        if (physical_address != 0) // If page is allocated
        {
            if (!(mem->flags & MAP_ANONYMOUS) && mem->flags & MAP_SHARED)
            {
                begin_op();
                ilock(mem->f->ip);
                writei(mem->f->ip, P2V((char *)physical_address), j*PGSIZE, PGSIZE);
                iunlock(mem->f->ip);
                end_op();
            }
            if (mem->ref == 0)
            {
                kfree(P2V((char *)physical_address));
            }
            
            *pte = 0; 
        }
    }

    // Remove the VMA from the array
    for (int k = i; k < MAX_VMAPS - 1; k++)
    {
        copy_vma_struct(&curproc->vmas[k], &curproc->vmas[k + 1]);
    }

    // Update total_vmaps and mark the last VMA as unmapped
    curproc->total_vmaps -= 1;
    curproc->vmas[MAX_VMAPS - 1].mapped = 0;

    return SUCCESS;
}

uint mywremap(uint oldaddr, int oldsize, int newsize, int flags)
{
    struct proc *curproc = myproc();
    struct vma *mem = 0;
    int i = 0;
    
    // Check for the mapped old address 
    for (i = 0; i < MAX_VMAPS; i++)
    {
        if (curproc->vmas[i].mapped == 1 && curproc->vmas[i].addr == oldaddr && curproc->vmas[i].length == oldsize)
        {
            // Assign mem with the vma that is mapped and contains the old 
            // starting address of the mapping
            mem = &curproc->vmas[i];
            break;
        }
    } 

    if (newsize <= 0) {
        cprintf("New size must be greater than 0.\n");
        return FAILED;
    }

    // If for-loop does not return a mapped vma with the 
    // exact address and length as oldaddr and oldsize
    if (mem == 0)
    {
        cprintf("Cannot remap non mapped addr\n");
        return FAILED;
    }

    if (oldsize - newsize > 0) 
    {
        // Shrink
        int offset = PGROUNDUP(newsize) / PGSIZE;
        int nuvmas = (mem->length - 1) / PGSIZE + 1;
        for (int j = offset; j < nuvmas; j++)
        {
            pte_t *pte = walkpgdir(curproc->pgdir, (void *)oldaddr + j * PGSIZE, 0);
            uint physical_address = PTE_ADDR(*pte);
            // cprintf("Unmapping virt: %x, phys: %x\n", addr + j * PGSIZE, physical_address);
            if (physical_address != 0) // If page is allocated
            {
                if (!(mem->flags & MAP_ANONYMOUS) && mem->flags & MAP_SHARED)
                {
                    begin_op();
                    ilock(mem->f->ip);
                    writei(mem->f->ip, P2V((char *)physical_address), j*PGSIZE, PGSIZE);
                    iunlock(mem->f->ip);
                    end_op();
                }
                kfree(P2V((char *)physical_address));
                *pte &= ~PTE_P; // No need to completely override the PTE, just make not present
            }
        }
        curproc->vmas[i].length = newsize;
        return oldaddr;
    }
    else 
    {
        // Position i of vmas is the current position
        int expand = 0;
        if (curproc->vmas[i+1].mapped == 0) 
        {
            expand = 1;
        } 
        else if ((oldaddr + newsize < curproc->vmas[i+1].addr))
        {
            expand = 1;
        }


        if (expand == 1)
        {
            curproc->vmas[i].length = newsize;
            curproc->vmas[i].end = (curproc->vmas[i].addr + newsize);
            return curproc->vmas[i].addr;
        }
        if (flags & MREMAP_MAYMOVE)
        {
            // int oldflags = curproc->vmas[i].flags; // Commented out because unused variable
            struct file* oldf = curproc->vmas[i].f;
            int loaded = curproc->vmas[i].num_loaded;
            
            for (int k = i; k < MAX_VMAPS - 1; k++)
            {
                copy_vma_struct(&curproc->vmas[k], &curproc->vmas[k + 1]);
            }
            int position = find_vmap_addr(curproc, newsize);
            curproc->vmas[position].end = (curproc->vmas[position].addr + newsize);
            curproc->vmas[position].length = newsize;
            curproc->vmas[position].flags = flags;
            curproc->vmas[position].f = oldf;
            curproc->vmas[position].mapped = 1;
            curproc->vmas[position].num_loaded = loaded;
            curproc->vmas[MAX_VMAPS - 1].mapped = 0;



            int nuvmas = (newsize - 1) / PGSIZE + 1;
            // cprintf("Going to map %d pages\n", nuvmas);
            for (int j = 0; j < nuvmas; j++)
            {

                pte_t *pte = walkpgdir(curproc->pgdir, (void *)oldaddr + j * PGSIZE, 0);
                uint physical_address = PTE_ADDR(*pte);
                
                
                if (physical_address != 0) // If page is allocated
                {
                    char *new = kalloc(); // Returns kernal allowed mem addr
                    memset(new, 0, PGSIZE);
                    if (!new)
                    {
                        cprintf("OOM\n");
                        myproc()->killed = 1;
                        return FAILED;
                    }
                    
                    mappages(curproc->pgdir, (void *)(PGROUNDDOWN(curproc->vmas[position].addr + j*PGSIZE)), (uint)PGSIZE, V2P(new), PTE_W | PTE_U);
                    // cprintf("Mapped 0x%x to %x", PGROUNDDOWN(curproc->vmas[position].addr + j*PGSIZE), new);
                    // cprintf("Moving 0x%x to 0x%x\n", P2V((char *)physical_address), curproc->vmas[position].addr+j*PGSIZE);
                    memmove(new, P2V((char *)physical_address), PGSIZE);
                    kfree(P2V((char *)physical_address));
                }
                *pte = 0;
            }
            return curproc->vmas[position].addr;

        }
    }
    
    
    return FAILED;
}
int mygetpgdirinfo(struct pgdirinfo *pdinfo)
{
    pde_t *pgdir = myproc()->pgdir;
    int count = 0;
    int i, j;
    pte_t *pgtab;

    for (i = 0; i < NPDENTRIES / 2; i++)
    {
        if (!(pgdir[i] & PTE_P))
            continue;
        pgtab = (pte_t *)P2V(pgdir[i] & ~0xFFF);
        for (j = 0; j < NPDENTRIES; j++)
        {
            if (pgtab[j] & PTE_P && pgtab[j] & PTE_U)
            {
                // cprintf("virt:%x phys:%x\n", (i << 22) + (j << 12), PTE_ADDR(pgtab[j]));
                // Note, is in virt addr assending order, can be used easily to add to va and pa
                if (count < MAX_UPAGE_INFO)
                {
                    pdinfo->va[count] = ((i << 22) + (j << 12));
                    pdinfo->pa[count] = PTE_ADDR(pgtab[j]);
                }
                count++;
            }
        }
    }
    pdinfo->n_upages = count;
    return SUCCESS;
}
int mygetwmapinfo(struct wmapinfo *wminfo)
{
    struct proc *curproc = myproc();
    wminfo->total_mmaps = curproc->total_vmaps;
    for (int i = 0; i < MAX_WMMAP_INFO; i++)
    {
        // cprintf("(%d) %x with %d)\n", i, (curproc->vmas[i]).addr, (((PGROUNDUP(curproc->vmas[i].length)) / PGSIZE)));
        wminfo->addr[i] = (curproc->vmas[i]).addr;
        wminfo->length[i] = (curproc->vmas[i]).length;
        wminfo->n_loaded_pages[i] = curproc->vmas[i].num_loaded;
    }
    // cprintf("\n");
    return SUCCESS;
}
