#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdbool.h>
#include "wfs.h"
#include <fuse.h>
int foreground = 0;
int verbose = 0; // More verbose printing
void *file_map = 0;
struct wfs_sb *super = 0;

void set_bitmap_bit(char *bitmap, off_t index)
{
    bitmap[index / 8] |= (1 << (index % 8));
}

static void clear_bitmap_bit(char *bitmap, off_t bit_offset)
{
    bitmap[bit_offset / 8] &= ~(1 << bit_offset % 8);
}

// Check if the bit at the given index in the bitmap is set
bool is_bitmap_bit_set(char *bitmap, off_t index)
{
    return (bitmap[index / 8] & (1 << (index % 8))) != 0;
}

struct wfs_inode *get_inode_by_number(int inode_number)
{
    // Assuming file_map and super are accessible globally
    return (struct wfs_inode *)((char *)file_map + super->i_blocks_ptr + (inode_number * BLOCK_SIZE));
}

struct wfs_dentry *get_directory_entry(int block_number)
{
    // Assuming file_map is accessible globally
    return (struct wfs_dentry *)((char *)file_map + block_number);
}

/**
 * Given a path, return the index of the inode, note that -1 denotes an error (often file not found)
 */
int path2inode(const char *path)
{
    if (strlen(path) == 0)
    {
        return 0;
    }

    if (strcmp(path, "/") == 0)
    { // Root inode
        printf("returned root\n");
        return 0;
    }

    // Traverse the path to find the corresponding inode
    char *token;
    char *path_copy = strdup(path);
    if (path_copy == NULL)
    {
        printf("Memory allocation failed for path copy.\n");
        return -ENOMEM; // Use standard error code for no memory
    }
    int final_found = 0;
    int found = 0;
    int inodeIndex = 0; // Start from the root inode
                        //   int subInodes[D_BLOCK] = {0};
    char *last_slash = strrchr(path, '/');
    struct wfs_inode *inode = (struct wfs_inode *)((char *)file_map + super->i_blocks_ptr);
    while ((token = strtok_r(path_copy, "/", &path_copy)))
    {
        found = 0;
        printf("Processing token: %s\n", token);
        for (int i = 0; i < N_BLOCKS; i++)
        {
            if (inode->blocks[i] != 0)
            {
                struct wfs_dentry *entry = get_directory_entry(inode->blocks[i]);
                for (int j = 0; j < BLOCK_SIZE / sizeof(struct wfs_dentry); j++)
                {
                    printf("Comparing Entry:%s Token:%s\n", entry[j].name, token);
                    if (strcmp(entry[j].name, token) == 0)
                    {
                        if (strcmp(last_slash + 1, entry[j].name) == 0)
                        {
                            final_found = 1;
                        }
                        printf("Match found for entry: %s\n", entry[j].name);
                        inode = get_inode_by_number(entry[j].num);
                        inodeIndex = entry[j].num;
                        found = 1;
                        break;
                    }
                }
            }
            if (found == 1)
                break;
        }
        if (final_found == 1)
            break;
        
    }
    if (found == 0 || final_found == 0 || !is_bitmap_bit_set((char *)(super->i_bitmap_ptr + (off_t)file_map), inode->num))
    {
        printf("Inode not found or invalid.\n");
        return -ENOENT;
    }

    printf("Inode details:\n");
    printf("Inode number: %d\n", inode->num);
    printf("Inode mode: %d\n", inode->mode);
    printf("Inode links: %d\n", inode->nlinks);
    printf("Inode size: %ld\n", inode->size);
    // free(path_copy);
    return inodeIndex;
}

/**
 * Goes through the iNode bitmap and gets the firest available position, it reservers this position
 */
int getEmptyInode()
{
    for (size_t i = 0; i < super->num_inodes; i++)
    {
        if (!is_bitmap_bit_set((char *)(super->i_bitmap_ptr + (off_t)file_map), i))
        {
            set_bitmap_bit(((char *)(super->i_bitmap_ptr + (off_t)file_map)), i);
            return i;
        }
    }
    return -1;
}

/**
 * Allocate a new data block from the data bitmap
 */
static int allocateDataBlock()
{
    char *data_bitmap = (char *)super->d_bitmap_ptr + (off_t)file_map;
    for (size_t i = 0; i < super->num_data_blocks; i++)
    {
        if (!is_bitmap_bit_set(data_bitmap, i))
        {
            // Mark the data block as used
            set_bitmap_bit(data_bitmap, i);
            printf("Allocated datablock at %x\n", (int)(i * BLOCK_SIZE) + (int)(super->d_blocks_ptr));
            return (int)(i * BLOCK_SIZE) + (int)(super->d_blocks_ptr);
        }
    }
    return -ENOSPC; // No space available
}

static int isDirectory(int inodeIndex)
{
    if (inodeIndex < 0 || inodeIndex >= super->num_inodes)
    {
        return 0; // Invalid inode index
    }

    struct wfs_inode *inode = get_inode_by_number(inodeIndex);
    return (inode->mode & S_IFDIR) != 0;
}

static int addDirectoryEntry(struct wfs_inode *parent_inode_ptr, const char *name, int new_inodeNum, time_t currTime)
{
    for (int i = 0; i < N_BLOCKS; i++)
    {
        if (parent_inode_ptr->blocks[i] != 0)
        {
            struct wfs_dentry *entries = get_directory_entry(parent_inode_ptr->blocks[i]);
            for (int j = 0; j < BLOCK_SIZE / sizeof(struct wfs_dentry); j++)
            {
                if (entries[j].num == 0)
                {
                    if (snprintf(entries[j].name, MAX_NAME, "%s", name) >= MAX_NAME)
                    {
                        return -ENAMETOOLONG;
                    }
                    entries[j].num = new_inodeNum;
                    parent_inode_ptr->mtim = currTime;
                    return 0;
                }
            }
        }
        else
        {
            off_t block_num = allocateDataBlock();
            if (block_num == -ENOSPC)
            {
                return -ENOSPC;
            }
            parent_inode_ptr->blocks[i] = block_num;
            struct wfs_dentry *entries = get_directory_entry(block_num);
            if (snprintf(entries[0].name, MAX_NAME, "%s", name) >= MAX_NAME)
            {
                return -ENAMETOOLONG;
            }
            entries[0].num = new_inodeNum;
            parent_inode_ptr->mtim = currTime;
            return 0;
        }
    }

    return -ENOSPC;
}

static int createNewEntry(const char *path, mode_t mode)
{
    // Get the parent directory's inode index
    char parent_path[strlen(path) + 1];
    strcpy(parent_path, path);
    char *last_slash = strrchr(parent_path, '/');
    if (last_slash == NULL)
    {
        // Invalid path
        return -EINVAL;
    }
    *last_slash = '\0'; // Terminate the parent directory path
    int parent_inode = path2inode(parent_path);
    if (parent_inode == -1)
    {
        // Parent directory not found
        return -ENOENT;
    }

    // Ensure that the parent directory is writable
    struct wfs_inode *parent_inode_ptr = get_inode_by_number(parent_inode);
    if (!(parent_inode_ptr->mode & S_IWUSR))
    {
        // Parent directory not writable
        return -EACCES;
    }
    // Find an empty inode for the new file or directory
    int new_inodeNum = getEmptyInode();
    if (new_inodeNum == -1)
    {
        // No empty inode available
        return -ENOSPC;
    }

    // Create a new inode for the file or directory
    struct wfs_inode *new_inode = get_inode_by_number(new_inodeNum);
    new_inode->num = new_inodeNum; // New inode number
    new_inode->mode = mode;        // Set mode for the new file or directory
    new_inode->nlinks = 1;         // Initially, one link to itself
    new_inode->uid = getuid();
    new_inode->gid = getgid();
    new_inode->size = 0;
    time_t currTime = time(NULL);
    new_inode->atim = currTime;
    new_inode->mtim = currTime;
    new_inode->ctim = currTime;
    if (addDirectoryEntry(parent_inode_ptr, last_slash + 1, new_inodeNum, currTime) != 0)
    {
        return -ENOSPC;
    }
    return 0;
}

static int releaseDataBlocks(struct wfs_inode *inode)
{
    for (size_t i = 0; i < D_BLOCK; i++)
    {
        if (inode->blocks[i] != 0)
        {
            // Clear the data block from the data bitmap
            memset((char *)inode->blocks[i] + (off_t)file_map, 0, BLOCK_SIZE);
            printf("Bitoffset for remove: %ld\n", (inode->blocks[i] - super->d_blocks_ptr) / BLOCK_SIZE);
            clear_bitmap_bit((char *)super->d_bitmap_ptr + (off_t)file_map, (inode->blocks[i] - super->d_blocks_ptr) / BLOCK_SIZE);
            // Reset the data block in the inode
            inode->blocks[i] = 0;
        }
    }
    if (inode->blocks[IND_BLOCK] != 0)
    {
        off_t *indirect = (off_t *)(inode->blocks[IND_BLOCK] + (off_t)file_map);
        for (size_t i = 0; i < BLOCK_SIZE / sizeof(off_t); i++)
        {
            if (indirect[i] == 0)
            {
                continue;
            }

            clear_bitmap_bit((char *)super->d_bitmap_ptr + (off_t)file_map, (indirect[i] - super->d_blocks_ptr) / BLOCK_SIZE);
            printf("Bitoffset for remove indirect: %ld\n", (indirect[i] - super->d_blocks_ptr) / BLOCK_SIZE);

            memset((char *)indirect[i] + (off_t)file_map, 0, BLOCK_SIZE);
        }
        memset(indirect, 0, BLOCK_SIZE);
        printf("Bitoffset for remove of Indirect block: %ld\n", (inode->blocks[IND_BLOCK] - super->d_blocks_ptr) / BLOCK_SIZE);

        clear_bitmap_bit((char *)super->d_bitmap_ptr + (off_t)file_map, (inode->blocks[IND_BLOCK] - super->d_blocks_ptr) / BLOCK_SIZE);
    }

    return 0; // Success
}

/**
 * Remove directory entry corresponding to the given path
 */
static int removeDirectoryEntry(const char *path)
{
    char parent_path[strlen(path) + 1];
    strcpy(parent_path, path);
    char *last_slash = strrchr(parent_path, '/');
    if (last_slash == NULL)
    {
        return -EINVAL;
    }
    *last_slash = '\0';
    int parent_inode = path2inode(parent_path);
    if (parent_inode == -1)
    {
        return -ENOENT;
    }

    struct wfs_inode *parent_inode_ptr = get_inode_by_number(parent_inode);
    if (!(parent_inode_ptr->mode & S_IWUSR))
    {
        return -EACCES;
    }

    int found = 0;
    for (size_t i = 0; i < D_BLOCK; i++)
    {
        struct wfs_dentry *entries = get_directory_entry(parent_inode_ptr->blocks[i]);

        for (int j = 0; j < BLOCK_SIZE / sizeof(struct wfs_dentry); j++)
        {
            if (entries[j].num != 0)
            {
                if (strcmp(entries[j].name, last_slash + 1) == 0)
                {
                    entries[j].num = 0;
                    memset(entries[j].name, 0, MAX_NAME);
                    found = 1;
                    break;
                }
            }
        }
        if (found) break; // Exit the loop if the entry is found
    }

    if (!found)
    {
        return -ENOENT;
    }

    return 0; // Success
}


/**
 * Must fill out the following items:
 * st_uid, st_gid, st_atime, st_mtime, st_mode, and st_size
 */
static int wfs_getattr(const char *path, struct stat *stbuf)
{
    printf("Getattr) path: %s\n", path);
    int inodeNum = path2inode(path);
    if (inodeNum == -2)
    {
        printf("Get attr returned -2\n");
        return -2;
    }
    struct wfs_inode *inode = get_inode_by_number(inodeNum);
    // printf("HERE!!!\n");
    // printf("%p\n", (void *) &inode->mode);
    stbuf->st_uid = inode->uid;
    stbuf->st_gid = inode->gid;
    stbuf->st_atime = inode->atim;
    stbuf->st_mtime = inode->mtim;
    stbuf->st_mode = inode->mode;
    stbuf->st_size = inode->size;
    printf("Get attr returned 0\n");
    return 0;
}

/**
 * Create a new file at the specified path
 */
static int wfs_mknod(const char *path, mode_t mode, dev_t rdev)
{
    printf("mknod) mode: %u path: %s ", mode, path);

    // Create a new file at the specified path with the given mode
    int result = createNewEntry(path, mode);
    if (result != 0)
    {
        // Error creating file
        return result;
    }
    printf("result %d\n", result);
    return 0; // Success
}

/**
 * Create a new directory at the specified path
 */
static int wfs_mkdir(const char *path, mode_t mode)
{
    printf("mkdir) mode: %u path: %s\n", mode, path);

    // Create a new directory at the specified path with the given mode
    int result = createNewEntry(path, mode | S_IFDIR); // Set the directory flag
    if (result != 0)
    {
        // Error creating directory
        return result;
    }

    return 0; // Success
}

static int wfs_unlink(const char *path)
{
    printf("unlink) path: %s\n", path);
    int file_inode = path2inode(path);
    if (file_inode == -1)
    {
        // File not found
        return -ENOENT;
    }

    struct wfs_inode *file_inode_ptr = get_inode_by_number(file_inode);
    // Ensure that the file is writable
    if (!(file_inode_ptr->mode & S_IWUSR))
    {
        // File not writable
        return -EACCES;
    }

    // Release data blocks associated with the file inode
    int result = releaseDataBlocks(file_inode_ptr);
    if (result != 0)
    {
        // Error releasing data blocks
        return result;
    }

    // Remove directory entry corresponding to the given path
    result = removeDirectoryEntry(path);
    if (result != 0)
    {
        // Error removing directory entry
        return result;
    }
    clear_bitmap_bit((char *)super->i_bitmap_ptr + (off_t)file_map, file_inode);



    return 0; // Success
}

static int wfs_rmdir(const char *path)
{
    printf("rmdir) path: %s\n", path);

    // Check if the last component of the path is "."
    char *last_slash = strrchr(path, '/');
    if (last_slash != NULL && strcmp(last_slash + 1, ".") == 0)
    {
        // Error: Cannot remove the current directory
        return -EINVAL;
    }

    // Get the inode index of the directory to remove
    int dir_inode = path2inode(path);
    if (dir_inode == -1)
    {
        // Directory not found
        return -ENOENT;
    }

    // Get the inode pointer for the directory
    struct wfs_inode *dir_inode_ptr = get_inode_by_number(dir_inode);

    // Ensure that the directory is writable
    if (!(dir_inode_ptr->mode & S_IWUSR))
    {
        // Directory not writable
        return -EACCES;
    }
    for (size_t i = 0; i < D_BLOCK; i++)
    {
        if (dir_inode_ptr->blocks[i] != 0)
        {
            printf("Clearing bitmap at position %ld\n", (dir_inode_ptr->blocks[i] - super->d_blocks_ptr) / BLOCK_SIZE);
            clear_bitmap_bit((char *)super->d_bitmap_ptr + (off_t)file_map, (dir_inode_ptr->blocks[i] - super->d_blocks_ptr) / BLOCK_SIZE);
        }
    }

    // Remove directory entry from parent directory
    int result = removeDirectoryEntry(path);
    if (result != 0)
    {
        // Error removing directory entry
        return result;
    }

    clear_bitmap_bit((char *)super->i_bitmap_ptr + (off_t)file_map, dir_inode);


    return 0; // Success
}


static off_t *get_block_ptr(struct wfs_inode *file_inode_ptr, off_t block_index, int allocate)
{
    printf("get_block_ptr: block_index = %ld, allocate = %d\n", block_index, allocate);

    if (block_index < D_BLOCK)
    {
        // Direct block access
        printf("Accessing direct block at index %ld\n", block_index);
        if (file_inode_ptr->blocks[block_index] == 0 && allocate)
        {
            printf("Direct block not allocated, attempting to allocate...\n");
            // Allocate a new data block if it's not already allocated
            int dataIndex = allocateDataBlock();
            if (dataIndex == -ENOSPC)
            {
                printf("No space available for data block, returning NULL\n");
                // No space available for data block
                return NULL;
            }
            file_inode_ptr->blocks[block_index] = dataIndex;
            printf("Allocated new data block at index %d\n", dataIndex);
        }
        else if (file_inode_ptr->blocks[block_index] == 0)
            return NULL;
        return (off_t *)(file_inode_ptr->blocks[block_index] + (off_t)file_map);
    }
    else
    {
        // Indirect block access
        off_t indirect_block_index = block_index - D_BLOCK;
        printf("Accessing indirect block at index %ld\n", indirect_block_index);
        if (indirect_block_index >= BLOCK_SIZE / sizeof(off_t))
        {
            printf("Invalid block index, returning NULL\n");
            // Invalid block index
            return NULL;
        }

        // Check if the indirect block is allocated
        if (file_inode_ptr->blocks[IND_BLOCK] == 0)
        {
            if (!allocate)
            {
                printf("Indirect block not allocated and allocation not allowed, returning NULL\n");
                // Indirect block not allocated and we're not allowed to allocate
                return NULL;
            }

            printf("Indirect block not allocated, attempting to allocate...\n");
            // Allocate a new indirect block
            int ind_block = allocateDataBlock();
            if (ind_block == -ENOSPC)
            {
                printf("No space available for indirect block, returning NULL\n");
                // No space available for indirect block
                return NULL;
            }
            file_inode_ptr->blocks[IND_BLOCK] = ind_block;
            // Clear the new indirect block
            memset((char *)((ind_block) + (off_t)file_map), 0, BLOCK_SIZE);
            printf("Allocated new indirect block at index %d\n", ind_block);
        }

        // Get the pointer to the indirect block
        off_t *indirect_block_ptr = (off_t *)((file_inode_ptr->blocks[IND_BLOCK]) + (off_t)file_map);

        // Check if the entry within the indirect block is allocated, if not, allocate a new data block
        if (indirect_block_ptr[indirect_block_index] == 0 && allocate)
        {
            printf("Indirect block entry not allocated, attempting to allocate...\n");
            int dataIndex = allocateDataBlock();
            if (dataIndex == -ENOSPC)
            {
                printf("No space available for data block, returning NULL\n");
                // No space available for data block
                return NULL;
            }
            indirect_block_ptr[indirect_block_index] = dataIndex;
            printf("Allocated new data block at index %d for indirect block\n", dataIndex);
        }

        // Point to the data block within the indirect block
        return (off_t *)(indirect_block_ptr[indirect_block_index] + (off_t)file_map);
    }
}

/**
 * Read data from a file
 */
static int wfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printf("read) path: %s\n", path);

    // Get the inode index of the file
    int file_inode = path2inode(path);
    if (file_inode == -ENOENT)
    {
        // File not found
        return -ENOENT;
    }

    // Get the inode pointer for the file
    struct wfs_inode *file_inode_ptr = get_inode_by_number(file_inode);

    // Check if the file is a regular file
    if (!(file_inode_ptr->mode & S_IFREG))
    {
        // Not a regular file
        return -EISDIR;
    }

    // Ensure that the offset is within the file size
    if (offset >= file_inode_ptr->size)
    {
        // Offset exceeds file size
        return 0;
    }

    // Adjust size if necessary
    size_t bytes_to_read = size;
    if (offset + size > file_inode_ptr->size)
    {
        bytes_to_read = file_inode_ptr->size - offset;
    }

    // Read data from the file
    size_t bytes_read = 0;
    size_t remaining_bytes = bytes_to_read;
    off_t current_offset = offset;

    while (bytes_read < bytes_to_read)
    {
        // Calculate block index and offset within block
        off_t block_index = current_offset / BLOCK_SIZE;
        off_t block_offset = current_offset % BLOCK_SIZE;

        // Check if we need to read from an indirect block
        off_t *block_ptr = get_block_ptr(file_inode_ptr,block_index, 0);
        if (block_ptr == NULL)
        {
            printf("Attempted to read a block that is not allocated\n");
            return -EIO;
        }
        


        // Read data from the block
        size_t bytes_to_copy = BLOCK_SIZE - block_offset;
        if (bytes_to_copy > remaining_bytes)
        {
            bytes_to_copy = remaining_bytes;
        }

        memcpy(buf + bytes_read, ((char *)block_ptr) + block_offset, bytes_to_copy);

        // Update counters
        bytes_read += bytes_to_copy;
        remaining_bytes -= bytes_to_copy;
        current_offset += bytes_to_copy;
    }

    return bytes_read; // Success
}

/**
 * Write data to a file
 */
static int wfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printf("write) path: %s, size %ld, offset %ld\n", path, size, offset);

    // Get the inode index of the file
    int file_inode = path2inode(path);
    if (file_inode == -1)
    {
        // File not found
        return -ENOENT;
    }

    // Get the inode pointer for the file
    struct wfs_inode *file_inode_ptr = get_inode_by_number(file_inode);

    // Check if the file is a regular file
    if (!(file_inode_ptr->mode & S_IFREG))
    {
        // Not a regular file
        return -EISDIR;
    }

    // Ensure that the offset is within the file size
    if (offset > file_inode_ptr->size)
    {
        // Offset exceeds file size
        return -EFBIG;
    }

    // Calculate the new size of the file after writing
    off_t new_size = offset + size;
    if (new_size > file_inode_ptr->size)
    {
        // Update the size of the file
        file_inode_ptr->size = new_size;
        printf("Updated size of file to %ld\n", new_size);
    }

    // Write data to the file
    size_t bytes_written = 0;
    size_t remaining_bytes = size;
    off_t current_offset = offset;

    while (bytes_written < size)
    {
        // Calculate block index and offset within block
        off_t block_index = current_offset / BLOCK_SIZE;
        off_t block_offset = current_offset % BLOCK_SIZE;

        // Allocate a new data block if necessary
        if (block_index >= N_BLOCKS + BLOCK_SIZE / sizeof(off_t))
        {
            // Invalid block index
            return -EFBIG;
        }

        off_t *block_ptr = get_block_ptr(file_inode_ptr, block_index, 1);
        if (block_ptr == NULL)
        {
            return -ENOSPC;
        }
        

        // Write data to the block
        size_t bytes_to_copy = BLOCK_SIZE - block_offset;
        if (bytes_to_copy > remaining_bytes)
        {
            bytes_to_copy = remaining_bytes;
        }

        printf("Attempting to write to %p\n", ((char *)(block_ptr)) + block_offset);
        memcpy(((char *)(block_ptr)) + block_offset, buf + bytes_written, bytes_to_copy);

        // Update counters
        bytes_written += bytes_to_copy;
        remaining_bytes -= bytes_to_copy;
        current_offset += bytes_to_copy;
    }

    return bytes_written; // Success
}

static int wfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    // Get the inode index corresponding to the given path
    int inodeIndex = path2inode(path);
    if (inodeIndex == -2)
    {
        // Directory not found
        return -ENOENT;
    }

    // Ensure that the given path is a directory
    if (!isDirectory(inodeIndex))
    {
        // Not a directory
        return -ENOTDIR;
    }
    struct wfs_inode *parent_inode = get_inode_by_number(inodeIndex);
    filler(buf, ".", NULL, 0);  // Current directory
    filler(buf, "..", NULL, 0); // Parent directory
    for (int i = 0; i < N_BLOCKS; i++)
    {
        if (parent_inode->blocks[i] != 0)
        {
            struct wfs_dentry *entries = get_directory_entry(parent_inode->blocks[i]);
            for (int j = 0; j < BLOCK_SIZE / sizeof(struct wfs_dentry); j++)
            {
                if (entries[j].num != 0)
                {
                    filler(buf, entries[j].name, NULL, 0);
                    printf("Found subdirectory %s\n", entries[j].name);
                }
            }
        }
    }

    return 0; // Success
}

static struct fuse_operations ops = {
    .getattr = wfs_getattr,
    .mknod = wfs_mknod,
    .mkdir = wfs_mkdir,
    .unlink = wfs_unlink,
    .rmdir = wfs_rmdir,
    .read = wfs_read,
    .write = wfs_write,
    .readdir = wfs_readdir,
};

int main(int argc, char *argv[])
{
    int fd = open(argv[1], O_RDWR);
    if (fd == -1)
    {
        perror("Error opening file");
        return 1;
    }
    struct stat st;
    if (fstat(fd, &st) == -1)
    {
        perror("Error getting file size");
        close(fd);
        return 1;
    }
    file_map = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (file_map == MAP_FAILED)
    {
        perror("Error mapping file into memory");
        close(fd);
        return 1;
    }
    super = file_map;

    return fuse_main(argc - 1, &argv[1], &ops, NULL);
}
