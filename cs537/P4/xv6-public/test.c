#include "types.h"
#include "user.h"
#include "stat.h"
#include "fs.h"
#include "fcntl.h"

#include "wmap.h"

int main()
{
    int length = 2 * 4096 + 1;
    uint addr = 0x60000000 + 4096 * 2;

    struct pgdirinfo pinfo0;
    getpgdirinfo(&pinfo0);
    printf(1, "Num mapped pages: %d\n", pinfo0.n_upages);
    
    int file = open("README", O_RDWR);
    printf(1, "file num: %d\n", file);
    uint map = wmap(0x60000000, length, MAP_PRIVATE, file);
    if (map != 0x60000000)
    {
        printf(1, "Got: %x Expected: 0x60000000\n", map);
    }
    
    {
        uint fail = wmap(0x80000000, 4096 + 8, MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED, 0);
        if (fail != 0xFFFFFFFF)
        {
                printf(1, "Got: %x Expected: FFFFFFFF\n", fail);
        }
        
    }
    {
        uint fail = wmap(0x50000000, 4096 + 8, MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED, 0);
        if (fail != 0xFFFFFFFF)
        {
                printf(1, "Got: %x Expected: FFFFFFFF\n", fail);
        }
        
    }

    char *arr = (char *)map;
    char val = 'p';
    for (int i = 0; i < length; i++)
    {
        if (arr[i] != 0)
        {
            printf(1, "%s\n", arr[i]);
        }
        arr[i] = val;
    }
    for (int i = 0; i < length; i++)
    {
        arr[i] = val;
    }
    // validate all pages of map 1
    for (int i = 0; i < length; i++)
    {
        if (arr[i] != val)
        {
            printf(1, "Cause: expected %c at 0x%x, but got %c\n", val, addr + i, arr[i]);
            exit();
        }
    }
    printf(1, "Map3 Fixed\n");
    uint map3 = wmap(0x60004000, length, MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, 0);
    if (map3 != 0x60004000)
    {
        printf(1, "Got: %x Expected: 0x60004000\n", map3);
    }

    
    struct wmapinfo winfo2;
    getwmapinfo(&winfo2);
    printf(1, "Total maps in use: %d\n", winfo2.total_mmaps);
    for (int i = 0; i < MAX_WMMAP_INFO; i++)
    {
        printf(1, "(va: %x, len: %d) ", winfo2.addr[i], winfo2.length[i]);
    }

    printf(1, "Map2\n");
    uint map2 = wmap(-2, 4096, MAP_PRIVATE | MAP_ANONYMOUS, 0);
    if (map2 != 0x60003000)
    {
        printf(1, "Got: %x Expected: 0x60003000\n", map2);
    }
    
    
    struct pgdirinfo pinfo1;
    getpgdirinfo(&pinfo1);
    printf(1, "Num mapped pages: %d\n", pinfo1.n_upages);
    for (int i = 0; i < pinfo1.n_upages; i++) {
        if (i == 32) {
            break;
        }
        printf(1, "virt:%x phys:%x\n", pinfo1.va[i], pinfo1.pa[i]);
    }


    getwmapinfo(&winfo2);
    printf(1, "Total maps in use: %d\n", winfo2.total_mmaps);

    for (int i = 0; i < MAX_WMMAP_INFO; i++)
    {
        printf(1, "(va: %x, len: %d) ", winfo2.addr[i], winfo2.length[i]);
    }
    printf(1, "\n");

    wunmap(map);
    close(file);
    wunmap(map3);

    getwmapinfo(&winfo2);
    printf(1, "Total maps in use after unmap: %d\n", winfo2.total_mmaps);
    for (int i = 0; i < MAX_WMMAP_INFO; i++)
    {
        printf(1, "(va: %x, len: %d) ", winfo2.addr[i], winfo2.length[i]);
    }
    printf(1, "\n");

    struct pgdirinfo pinfo2;
    getpgdirinfo(&pinfo2);
    printf(1, "Num mapped pages: %d\n", pinfo2.n_upages);
    wunmap(map2);
    exit();
}