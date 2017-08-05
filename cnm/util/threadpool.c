#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

#include "threadpool.h"

#define MAX_THREADPOOL_SIZE 128
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#define ngx_delete_posted_event(ev) ngx_queue_remove(&(ev)->queue)

static pthread_mutex_t mutex;
static pthread_cond_t cond;
static unsigned int idle_threads;
static unsigned int nthreads;
static pthread_mutex_t *threads;
static pthread_t default_threads[4];
static ngx_queue_t wq;
static unsigned int idle_threads;

static void worker(void *arg)
{
	ngx_queue_t *q;

	(void)arg;

	for (;;) {
		pthread_mutex_lock(&mutex);

		while (ngx_queue_empty(&wq)) {
			idle_threads += 1;
			pthread_cond_wait(&cond, &mutex);
			idle_threads -= 1;
		}

		q = ngx_queue_head(&wq);
		ae_work_t *worker = ngx_queue_data(q, ae_work_t, queue);
		ngx_queue_remove(&worker->queue);

		pthread_mutex_unlock(&mutex);
		worker->work(worker);
	}
}

void ae_init_threads(void)
{
	unsigned int i;
	int err;

	nthreads = ARRAY_SIZE(default_threads);
	idle_threads = 0;

	threads = (pthread_mutex_t *)default_threads;
	nthreads = ARRAY_SIZE(default_threads);

	pthread_cond_init(&cond, NULL);
	pthread_mutex_init(&mutex, NULL);

	ngx_queue_init(&wq);

	for (i = 0; i < nthreads; i++) {
		pthread_t *tid = (pthread_t *)(threads + i);
		err = pthread_create(tid, NULL, (void *)worker, NULL);
		if (err) {
			abort();
		}
	}
}

void ae_work_submit(ae_work_t *w,
					void (*work)(ae_work_t *w),
					void (*done)(ae_work_t *w, int status))
{
	w->work = work;
	w->done = done;

	pthread_mutex_lock(&mutex);
	ngx_queue_insert_tail(&wq, &w->queue);
	if (idle_threads > 0) {
		pthread_cond_signal(&cond);
	}
	pthread_mutex_unlock(&mutex);

	/*
	ngx_queue_t *q;
	for (q = ngx_queue_head(&wq); q != ngx_queue_sentinel(&wq); q = ngx_queue_next(q))
	{
		ae_work_t *w = ngx_queue_data(q, ae_work_t, queue);
	}
	printf("\r\n------------\r\n");
	*/
}
