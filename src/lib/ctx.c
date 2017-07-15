#include "ctx.h"
#include "error.h"
#include "shared/log.h"

#include <stdlib.h>
#include <signal.h>
#include <assert.h>
#include <stdio.h> // for perror

/**
 * Wrapper around pthread_mutex_unlock for use with pthread_cleanup_push
 */
static void pt_mutex_unlock (void *arg)
{
    pthread_mutex_t *mutex = arg;

    log_debug("cleanup");

    assert(!pthread_mutex_unlock(mutex));
}

/**
 * Enqueue the given piece of work
 *
 * This function always succeeds.
 */
static void pt_work_enqueue (struct pt_ctx *ctx, struct pt_work *work)
{
    // acquire
    assert(!pthread_mutex_lock(&ctx->work_mutex));

    // enqueue work
    TAILQ_INSERT_TAIL(&ctx->work, work, ctx_work);

    // if there's a thread waiting, wake one up. Otherwise, some thread will find it once it finishes its current work
    assert(!pthread_cond_signal(&ctx->work_cond));

    // release
    assert(!pthread_mutex_unlock(&ctx->work_mutex));
}

/**
 * Dequeue a piece of work, returning true while there was still work to do
 */
static bool pt_work_dequeue (struct pt_ctx *ctx, struct pt_work **work_ptr)
{
    bool ret;

    // acquire, cancel-safe
    pthread_cleanup_push(pt_mutex_unlock, &ctx->work_mutex);
    assert(!pthread_mutex_lock(&ctx->work_mutex));

    // wait for work
    while (ctx->running && TAILQ_EMPTY(&ctx->work))
        // we can expect to get pthread_cancel'd here
        assert(!pthread_cond_wait(&ctx->work_cond, &ctx->work_mutex));

    // still got work?
    if (!TAILQ_EMPTY(&ctx->work)) {
        // pop work
        *work_ptr = TAILQ_FIRST(&ctx->work);
        TAILQ_REMOVE(&ctx->work, *work_ptr, ctx_work);

        log_debug("got work");

        ret = true;

    } else {
        assert(!ctx->running);

        // work empty, idle
        assert(!pthread_cond_signal(&ctx->idle_cond));

        log_debug("idle/dead");

        // no more work
        ret = false;
    }

    // release
    pthread_cleanup_pop(true);

    return ret;
}

/**
 * Wait for work queue to empty and workers to finish
 */
static void pt_work_shutdown (struct pt_ctx *ctx)
{
    // acquire
    assert(!pthread_mutex_lock(&ctx->work_mutex));

    // indicate to terminate
    ctx->running = false;

    // wake up any idle workers
    assert(!pthread_cond_broadcast(&ctx->work_cond));

    // wait for it to drain...
    while (!TAILQ_EMPTY(&ctx->work))
        assert(!pthread_cond_wait(&ctx->idle_cond, &ctx->work_mutex));

    // release
    assert(!pthread_mutex_unlock(&ctx->work_mutex));
}

/**
 * Execute a piece of work
 */
static void pt_work_execute (struct pt_work *work)
{
    work->func(work->arg);
}

/**
 * Release work state once done
 */
static void pt_work_release (struct pt_work *work)
{
    free(work);
}

/**
 * Worker thread entry point
 */
static void* pt_thread_main (void *arg)
{
    struct pt_thread *thread = arg;
    struct pt_work *work;

    log_debug("start");

    // if only life were so simple...
    while (pt_work_dequeue(thread->ctx, &work)) {
        log_debug("work_execute");
        pt_work_execute(work);
        pt_work_release(work);
        log_debug("work_done");
    }

    log_debug("exit");

    return NULL;
}

/**
 * Shut down a pt_thread, waiting for it to finish.
 *
 * Does not remove the thread from the pool or release the pt_thread.
 */
static void pt_thread_shutdown (struct pt_thread *thread)
{
    // wait for it to finish
    if (pthread_join(thread->tid, NULL))
        perror("pthread_join");

    // mark
    thread->tid = 0;
}

/**
 * Release pt_thread state, aborting thread if running.
 *
 * This is guaranteed to remove the thread from the ctx_threads if it was added.
 */
static void pt_thread_destroy (struct pt_thread *thread)
{
    if (thread->tid) {
        // detach
        if (pthread_detach(thread->tid))
            perror("pthread_detach");

        // abort thread
        if (pthread_kill(thread->tid, SIGTERM))
            perror("pthread_detach");

    }

    // remove from pool
    TAILQ_REMOVE(&thread->ctx->threads, thread, ctx_threads);

    free(thread);
}


/**
 * Start a new thread and add it to the thread pool
 */
static int pt_ctx_add_thread (struct pt_ctx *ctx)
{
    struct pt_thread *thread;
    int err;

    // alloc
    if ((thread = calloc(1, sizeof(*thread))) == NULL)
        return -PT_ERR_MEM;

    // init
    thread->ctx = ctx;

    // start thread, default attrs
    if (pthread_create(&thread->tid, NULL, pt_thread_main, thread))
        JUMP_SET_ERROR(err, PT_ERR_PTHREAD_CREATE);

    // add to pool
    TAILQ_INSERT_TAIL(&ctx->threads, thread, ctx_threads);

    // ok
    return 0;

error:
    // drop, don't try and remove from tailq...
    free(thread);

    return err;
}

int pt_ctx_new (struct pt_ctx **ctx_ptr, int threads)
{
    struct pt_ctx *ctx;
    int err;

    // alloc
    if ((ctx = calloc(1, sizeof(*ctx))) == NULL)
        return -PT_ERR_MEM;

    // init
    TAILQ_INIT(&ctx->threads);
    TAILQ_INIT(&ctx->work);
    pthread_mutex_init(&ctx->work_mutex, NULL);
    pthread_cond_init(&ctx->work_cond, NULL);
    pthread_cond_init(&ctx->idle_cond, NULL);
    ctx->running = true;

    // start threadpool
    for (int i = 0; i < threads; i++) {
        if ((err = pt_ctx_add_thread(ctx)))
            JUMP_ERROR(err);
    }

    // ok
    *ctx_ptr = ctx;

    return 0;

error:
    // cleanup
    pt_ctx_destroy(ctx);

    return err;
}

int pt_ctx_work (struct pt_ctx *ctx, pt_work_func func, void *arg)
{
    struct pt_work *work;

    // check state
    // XXX: this is kind of retarded, because pt_ctx_shutdown/work should only be called from the same thread...
    if (!ctx->running)
        RETURN_ERROR(PT_ERR_CTX_SHUTDOWN);

    // alloc
    if ((work = calloc(1, sizeof(*work))) == NULL)
        RETURN_ERROR(PT_ERR_MEM);

    // init
    work->func = func;
    work->arg = arg;

    // enqueue
    pt_work_enqueue(ctx, work);

    // ok
    return 0;
}

int pt_ctx_shutdown (struct pt_ctx *ctx)
{
    struct pt_thread *thread;

    // finish work
    pt_work_shutdown(ctx);

    // shut down each thread
    TAILQ_FOREACH(thread, &ctx->threads, ctx_threads)
        // XXX: handle errors of some kind?
        pt_thread_shutdown(thread);

    // then drop
    pt_ctx_destroy(ctx);

    return 0;
}

void pt_ctx_destroy (struct pt_ctx *ctx)
{
    struct pt_thread *thread;

    // kill threads
    while ((thread = TAILQ_FIRST(&ctx->threads)))
        pt_thread_destroy(thread);

    // destroy mutex/conds
    if (pthread_cond_destroy(&ctx->idle_cond))
        perror("pthread_cond_destroy(idle_cond)");

    if (pthread_cond_destroy(&ctx->work_cond))
        perror("pthread_cond_destroy(work_cond)");

    if (pthread_mutex_destroy(&ctx->work_mutex))
        perror("pthread_mutex_destroy(work_mutex)");


    free(ctx);
}
