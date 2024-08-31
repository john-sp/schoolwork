#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <getopt.h>
#include <sys/mman.h>
#include <stdbool.h>

#include "wfs.h" 

// Set the bit at the given index in the bitmap
void set_bitmap_bit(char *bitmap, off_t index) {
    bitmap[index / 8] |= (1 << (index % 8));
}

// Check if the bit at the given index in the bitmap is set
bool is_bitmap_bit_set(char *bitmap, off_t index) {
    return (bitmap[index / 8] & (1 << (index % 8))) != 0;
}

void print_usage(const char *program_name) {
    printf("Usage: %s -d <filename> -i <numInodes> -b <numBlocks>\n", program_name);
}

int main(int argc, char *argv[]) {
    char *filename = NULL;
    size_t numInodes = 0;
    size_t numBlocks = 0;

    int opt;
    while ((opt = getopt(argc, argv, "d:i:b:")) != -1) {
        switch (opt) {
            case 'd':
                filename = optarg;
                break;
            case 'i':
                numInodes = atoi(optarg);
                break;
            case 'b':
                numBlocks = atoi(optarg);
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (filename == NULL || numInodes == 0 || numBlocks == 0) {
        print_usage(argv[0]);
        return 1;
    }

    numBlocks = (numBlocks + 31) & ~(size_t)31;
    numInodes = (numInodes + 31) & ~(size_t)31;
    printf("Number of blocks: %ld, Num of inodes: %ld\n", numBlocks, numInodes);
    // Create a file with the specified filename
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return 1;
    }


    off_t sb_size = sizeof(struct wfs_sb);
    off_t i_bitmap_size = (numInodes + 7) / 8; // 1 bit per inode
    off_t d_bitmap_size = (numBlocks + 7) / 8; // 1 bit per data block
    off_t inode_size = numInodes * BLOCK_SIZE;
    off_t total_size = sb_size + i_bitmap_size + d_bitmap_size + inode_size + (BLOCK_SIZE * numBlocks);

    printf("sizes: sb: %ld, ibit: %ld, dbit: %ld, inode: %ld total %ld\n", sb_size, i_bitmap_size, d_bitmap_size, inode_size, total_size);

    // Get the current size of the file
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("Error getting file size");
        close(fd);
        return 1;
    }

    // Check if the file is large enough to accommodate the required size
    if (st.st_size < total_size) {
        printf("Error: File is too small to accommodate the specified number of blocks.\n");
        printf("Required size: %ld bytes\n", total_size);
        printf("Current size: %ld bytes\n", st.st_size);
        close(fd);
        return 1;
    }

    // Map the file into memory
    void *file_map = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (file_map == MAP_FAILED) {
        perror("Error mapping file into memory");
        close(fd);
        return 1;
    }

    // Write superblock
    struct wfs_sb *superblock = (struct wfs_sb *)file_map;
    superblock->num_inodes = numInodes;
    superblock->num_data_blocks = numBlocks;
    superblock->i_bitmap_ptr = sb_size;
    superblock->d_bitmap_ptr = sb_size + i_bitmap_size;
    superblock->i_blocks_ptr = sb_size + i_bitmap_size + d_bitmap_size;
    superblock->d_blocks_ptr = sb_size + i_bitmap_size + d_bitmap_size + inode_size;

    // Write inode bitmap and data block bitmap, initializing all bits to 0
    memset((char *)file_map + sb_size, 0, i_bitmap_size + d_bitmap_size);

    // Write inodes, initializing all values to 0
    struct wfs_inode *inodes = (struct wfs_inode *)((char *)file_map + sb_size + i_bitmap_size + d_bitmap_size);
    memset(inodes, 0, inode_size);
    printf("Position of inode 0: 0x%lx\n", (off_t)inodes - (off_t)file_map);
    // Set up root directory
    inodes[0].num = 0; // Root inode number
    inodes[0].mode = S_IFDIR | 0755; // Directory mode
    inodes[0].uid = getuid();
    inodes[0].gid = getuid();
    inodes[0].size = BLOCK_SIZE; // "." and ".." entries
    inodes[0].nlinks = 2; // "." entry links to itself
    time_t currTime = time(NULL);
    inodes[0].atim = currTime;
    inodes[0].mtim = currTime;
    inodes[0].ctim = currTime;
    // inodes[0].blocks[0] = sb_size + i_bitmap_size + d_bitmap_size + inode_size; // Start of data blocks for root directory

    // Update inode bitmap and data block bitmap for root directory
    set_bitmap_bit((char *)file_map + superblock->i_bitmap_ptr, 0); // Root inode

    // Clean up
    munmap(file_map, total_size);
    close(fd);

    printf("Filesystem created successfully!\n");

    return 0;
}
