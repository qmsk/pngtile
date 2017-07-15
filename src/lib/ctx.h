#ifndef PNGTILE_CTX_H
#define PNGTILE_CTX_H

/**
 * Shared context between images, used to provide a threadpool for parralelizing tile operations
 */
#include "pngtile.h"

#include <pthread.h>
#include <sys/queue.h>
#include <stdbool.h>

/**
 * Worker thread
 */
struct pt_thread {
    /** Shared context */
    struct pt_ctx *ctx;

    /** Thread handle */
    pthread_t tid;

    /** @see pt_ctx::threads */
    TAILQ_ENTRY(pt_thread) ctx_threads;
};

/**
 * Work function
 */
typedef void (*pt_work_func) (void *arg);

/**
 * Work that needs to be done
 */
struct pt_work {
    /** Work func */
    pt_work_func func;

    /** Work info */
    void *arg;

    /** @see pt_ctx::work */
    TAILQ_ENTRY(pt_work) ctx_work;
};

/**
 * Shared context
 */
struct pt_ctx {
    /** Accepting new work */
    bool running;

    /** Threadpool */
    TAILQ_HEAD(pt_ctx_threads, pt_thread) threads;

    /** Pending work */
    TAILQ_HEAD(pt_ctx_work, pt_work) work;

    /** Control access to ::work */
    pthread_mutex_t work_mutex;

    /** Wait for work to become available */
    pthread_cond_t work_cond;

    /** Thread is idle */
    pthread_cond_t idle_cond;
};

/**
 * Enqueue a work unit
 */
int pt_ctx_work (struct pt_ctx *ctx, pt_work_func func, void *arg);


#endif
