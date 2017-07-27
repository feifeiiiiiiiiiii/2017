#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include "queue.h"

typedef struct ae_work_s ae_work_t;

struct ae_work_s {
	void (*work)(ae_work_t *w);
	void (*done)(ae_work_t *w, int status);
	void *arg;
	ngx_queue_t queue;
};

void ae_work_submit(ae_work_t *w,
					void (*work)(ae_work_t *w),
					void (*done)(ae_work_t *w, int status));
void ae_init_threads(void);

#endif /* THREADPOOL_H_ */
