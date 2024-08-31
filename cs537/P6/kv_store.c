#include <stdio.h>
#include <sys/mman.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <semaphore.h>
#include <stdatomic.h>

#include "common.h"
#include "ring_buffer.h"

#define MAX_THREADS 128
int verbose = 0;
pthread_t threads[MAX_THREADS];

struct ring *ring = NULL;
char *shmem_area = NULL;
#define PRINTV(...)         \
    if (verbose)            \
        printf("Server: "); \
    if (verbose)            \
    printf(__VA_ARGS__)

// Define a node structure for chaining

typedef struct Node
{
    key_type key;
    value_type value;
    atomic_uintptr_t next; 
} Node;

// Define the bucket structure with atomic operations
typedef struct
{
    atomic_uintptr_t head;
} Bucket;

// Define the hash table structure
typedef struct
{
    atomic_size_t size;
    Bucket *buckets; 
} HashTable;

// Initialize the bucket
void init_bucket(Bucket *bucket)
{
    atomic_init(&bucket->head, (uintptr_t)NULL); // Initialize head to NULL
}

// Initialize the hash table
void init_hash_table(HashTable *ht, int size)
{
    atomic_init(&ht->size, size);
    ht->buckets = (Bucket *)malloc(size * sizeof(Bucket));
    for (int i = 0; i < size; ++i)
    {
        init_bucket(&ht->buckets[i]);
    }
}

// Insert a key-value pair into the hash table
void insert(HashTable *ht, key_type key, value_type value)
{

    size_t index = hash_function(key, atomic_load(&ht->size));
    Bucket *buckets = ht->buckets;
    Bucket *bucket = &buckets[index];

    Node *current = (Node *)atomic_load(&bucket->head);
    while (current != NULL)
    {
        if (current->key == key)
        {
            current->value = value;

            return; // Key found and updated, exit
        }
        current = (Node *)atomic_load(&current->next);
    }
    // Create a new node
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->key = key;
    newNode->value = value;

    // Atomically update the head pointer
    newNode->next = atomic_load(&bucket->head);
    while (!atomic_compare_exchange_strong(&bucket->head, &newNode->next, (uintptr_t)newNode))
        newNode->next = atomic_load(&bucket->head);

    // atomic_fetch_sub(&bucket->activeWriters, 1);
}

// Search for a key in the hash table
value_type search(HashTable *ht, key_type key)
{
    size_t index = hash_function(key, atomic_load(&ht->size));
    Bucket *buckets = ht->buckets;
    Bucket *bucket = &buckets[index];

    // Traverse the list in the bucket and search for the key
    Node *current = (Node *)atomic_load(&bucket->head);
    while (current != NULL)
    {
        if (current->key == key)
        {
            value_type result = current->value;
            // atomic_fetch_sub(&bucket->activeReaders, 1);
            return result;
        }
        current = (Node *)atomic_load(&current->next);
    }

    // atomic_fetch_sub(&bucket->activeReaders, 1);
    // Return a default value if key not found
    return 0;
}




HashTable *ht;

/*
    This function is used to insert a key-value pair into the store. If the key already exists, it updates the associated value.
*/
void put(key_type key, value_type val)
{
    insert(ht, key, val);
}

/*
    This function is used to retrieve the value associated with a given key from the store. If the key is not found, it returns 0.
*/
value_type get(key_type key)
{
    return search(ht, key);
}

void *thread_func(void *args)
{
    PRINTV("Thread Started\n");
    struct buffer_descriptor buf;
    while (1)
    {
        /* code */

        // PRINTV("Waiting for request\n");
        // printf("Sizes: mutex %d, cond %d\n", sizeof(pthread_mutex_t), sizeof(pthread_cond_t));

        ring_get(ring, &buf);
        // PRINTV("Addr of buf: %p, addr of ret: %p\n", (void *)&buf, (void *)(buf.res_off + shmem_area));

        // PRINTV("Got request with type: %d k:%u v: %u\n", buf.req_type, buf.k, buf.v);
        struct buffer_descriptor *out = (struct buffer_descriptor *)((buf.res_off + shmem_area));

        if (buf.req_type == PUT)
        {
            put(buf.k, buf.v);
            out->k = buf.k;
            PRINTV("Got PUT of key %d to offset %x, now ready\n", buf.k, buf.res_off);
            out->ready = 1;
        }
        else if (buf.req_type == GET)
        {
            key_type ret = get(buf.k);
            out->k = buf.k;
            out->v = ret;
            PRINTV("Got GET of key %d to offset %x, with return of %u now ready\n", buf.k, buf.res_off, ret);
            out->ready = 1;
        }
    }
}

int main(int argc, char *argv[])
{
    int num_threads = 0;
    int hashtable_size = 0;
    int opt;
    // Parse command line arguments
    while ((opt = getopt(argc, argv, "vn:s:")) != -1)
    {
        switch (opt)
        {
        case 'n':
            num_threads = atoi(optarg);
            break;
        case 's':
            hashtable_size = atoi(optarg);
            break;
        case 'v':
            verbose = 1;
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s -n num_threads -s hashtable_size\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    PRINTV("Args read, debug is on\n");

    // Check if required arguments are provided
    if (num_threads == 0 || hashtable_size == 0)
    {
        fprintf(stderr, "Both -n and -s options are required.\n");
        exit(EXIT_FAILURE);
    }
    ht = malloc(sizeof(HashTable));
    init_hash_table(ht, hashtable_size);

    int fd = open("shmem_file", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (fd < 0)
        perror("open");

    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        perror("fstat");
        exit(EXIT_FAILURE);
    }
    off_t shm_size = sb.st_size;

    char *mem = mmap(NULL, shm_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
    if (mem == (void *)-1)
        perror("mmap");

    /* mmap dups the fd, no longer needed */
    close(fd);

    ring = (struct ring *)mem;
    shmem_area = mem;
    PRINTV("Going to create %d threads\n", num_threads);
    for (int i = 0; i < num_threads; i++)
    {
        if (pthread_create(&threads[i], NULL, &thread_func, NULL))
            perror("pthread_create");
    }
    for (int i = 0; i < num_threads; i++)
    {
        if (pthread_join(threads[i], NULL))
            perror("pthread_join");
    }
    // thread_func();

    return 0;
}
