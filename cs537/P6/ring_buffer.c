#include "ring_buffer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdatomic.h>
#include <unistd.h>

#define MAX_WAIT_ITERATIONS 1000000

int init_ring(struct ring *r)
{
    if (r == NULL)
        return -1;

    memset(r, 0, sizeof(struct ring));

    // Initialize the synchronization primitives
    // if (pthread_mutex_init(&(r->mutex), NULL) != 0)
    //     return -2;
    // if (pthread_cond_init(&(r->not_full), NULL) != 0)
    //     return -3;
    // if (pthread_cond_init(&(r->not_empty), NULL) != 0)
    //     return -4;

    return 0;
}

void ring_submit(struct ring *r, struct buffer_descriptor *bd) {
    if (r == NULL || bd == NULL)
        return;

    // int count = 0;
    while (1)
    {
        int prod_head = atomic_load(&r->p_head);
        int cons_tail = atomic_load(&r->c_tail);

        int prod_next = (prod_head + 1);

        if (prod_next == cons_tail)
        {
            // Ring is full, cannot submit
            // sched_yield();
            continue;
        }

        // Attempt to update the producer head
        if (!atomic_compare_exchange_strong(&r->p_head, &prod_head, prod_next))
            continue; // Retry if failed due to contention

        // Write to the ring buffer
        r->buffer[prod_head % RING_SIZE] = *bd;
        atomic_thread_fence(memory_order_release);

        // Wait for other enqueues to complete
        while (atomic_load(&r->p_tail) != prod_head) {
            // printf("Waiting for other enqueues to complete\n");
            // usleep(10);
            // sched_yield();
        }
        // Update producer tail
        atomic_fetch_add(&r->p_tail, 1);

        break; // Exit the loop once successfully submitted
    }
    // printf("Client finished submit: Prod h: %d t: %d, Con h: %d t: %d\n",
    //        atomic_load(&r->p_head), atomic_load(&r->p_tail), atomic_load(&r->c_head), atomic_load(&r->c_tail));
}


void ring_get(struct ring *r, struct buffer_descriptor *bd) {
    if (r == NULL || bd == NULL)
        return;

    while (1)
    {
        int cons_head = atomic_load(&r->c_head);
        int prod_tail = atomic_load(&r->p_tail);

        int cons_next = (cons_head + 1);

        if (cons_head == prod_tail)
        {
            // printf("Waiting, ring is empty: Prod h: %d t: %d, Con h: %d t: %d\n",
            //        atomic_load(&r->p_head), prod_tail, cons_head, atomic_load(&r->c_tail));
            // usleep(10);
            // sched_yield();
            continue;
        }

        // Attempt to update the consumer head
        if (!atomic_compare_exchange_strong(&r->c_head, &cons_head, cons_next)){
            
            continue; // Retry if failed due to contention
        }

        *bd = r->buffer[cons_head % RING_SIZE];
        atomic_thread_fence(memory_order_acquire);



        // while(!atomic_compare_exchange_strong(&r->c_tail, cons_head, r->c_tail + 1)) {
            
        // }
        // Wait for other dequeues to complete
        while (atomic_load(&r->c_tail) != cons_head) {
            // printf("Waiting for other dequeues to complete\n");
            // sched_yield();
        }

        // Update consumer tail
        atomic_fetch_add(&r->c_tail, 1);
        // printf("Buffer retrieval successful\n");

        break; // Exit the loop once successfully dequeued
    }
}
