#include "threadpool.h"
#include <stdio.h>
#include <stdlib.h>

void work(ae_work_t *w) {
	printf("work idx = %d\n", w->arg);
}

void done(ae_work_t *w, int status) {
	printf("done idx = %d\n", w->arg);
}

int main()
{
	ae_init_threads();

	ae_work_t *w;
	int i;

	for (i = 0; i < 10000; ++i) {
		w = (ae_work_t *)malloc(sizeof(ae_work_t));
		w->arg = i;
		ae_work_submit(w, work, done);
	}

	sleep(1);
}
