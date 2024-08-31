# CS537-P4

## Group 26
### Members:

Name: Bahulya Tandon  
Email: btandon2@wisc.edu  
NetID: btandon2  
CS ID: bahulya  

Name: John Derr  
Email: jderr2@wisc.edu  
NetID: jderr2  
CS ID: jderr  

## Project status
- [x] First of all, make sure you understand how xv6 does memory management. Refer to the xv6 manual. The second chapter called 'page tables' gives a good insight of the memory layout in xv6. Furthermore, it references some related source files. Looking at those sources should help you understand how mapping happens. You'll need to use those routines while implementing wmap. You will appreciate the time you spent on this step later.
- [x] Try to implement a basic wmap. Do not worry about file-backed mapping at the moment, just try MAP_ANONYMOUS | MAP_FIXED | MAP_SHARED. It should just check if that particular region asked by the user is available or not. If it is, you should map the pages in that range.
- [x] Implement wunmap. For now, just remove the mappings.
- [x] Implement wmapinfo and getpgdirinfo. As mentioned earlier, most of the tests depend on these two syscalls to work.
- [x] Modify your wmap such that it's able to search for an available region in the process address space. This should make your wmap work without MAP_FIXED.
- [x] Support file-backed mapping. You'll need to change the wmap so that it's able to use the provided fd to find the file. You'll also need to revisit wunmap to write the changes made to the mapped memory back to disk when you're removing the mapping. You can assume that the offset is always 0.
- [X] Go for fork() and exit(). Copy mappings from parent to child across the fork() system call. Also, make sure you remove all mappings in exit().
- [x] Implement MAP_PRIVATE. You'll need to change 'fork()' to behave differently if the mapping is private. Also, you'll need to revisit 'wunmap' and make sure changes are NOT reflected in the underlying file with MAP_PRIVATE set.
- [x] Implement wremap.
- [x] Passes all the current tests. 
- [x] Finished.
