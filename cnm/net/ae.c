#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "ae.h"
#include "ae_epoll.c"

aeEventLoop *aeCreateEventLoop(int setsize) {
    aeEventLoop *eventLoop;
    int i;

    if((eventLoop = malloc(sizeof(*eventLoop))) == NULL) goto err;
    eventLoop->events = malloc(sizeof(aeFileEvent)*setsize);
    eventLoop->fired = malloc(sizeof(aeFireEvent)*setsize);
    if(eventLoop->events == NULL || eventLoop->fired == NULL) goto err;
    eventLoop->setsize = setsize;
    eventLoop->stop = 0;
    eventLoop->maxfd = -1;
    eventLoop->lastTime = time(NULL);
    if(aeApiCreate(eventLoop) == -1) goto err;

    for(i = 0; i < setsize; ++i) {
        eventLoop->events[i].mask = AE_NONE;
    }
		return eventLoop;
err:
  if(eventLoop) {
      free(eventLoop);
  }
  return NULL;
}

void aeDeleteEventLoop(aeEventLoop *eventLoop) {
		aeApiFree(eventLoop);
		free(eventLoop->events);
		free(eventLoop);
}

void aeStop(aeEventLoop *eventLoop) {
		eventLoop->stop = 1;
}

int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask, 
											aeFileProc *proc, void *clientData)
{
		if(fd >= eventLoop->setsize) {
				errno = ERANGE;
				return AE_ERR;
		}

		aeFileEvent *fe = &eventLoop->events[fd];
		
		if(aeApiAddEvent(eventLoop, fd, mask) == -1) return AE_ERR;

		fe->mask |= mask;
		if(mask & AE_READABLE) fe->rfileProc = proc;
		if(mask & AE_WRITABLE) fe->wfileProc = proc;
		fe->clientData = clientData;
		if(fd > eventLoop->maxfd) {
				eventLoop->maxfd = fd;
		}

		return AE_OK;
}

int aeProcessEvents(aeEventLoop *eventLoop, int flags) {
		int processed = 0, numevents;

		if(eventLoop->maxfd == -1) return 0;

		// TODO time
		numevents = aeApiPoll(eventLoop, NULL);

		int j;
		for (j = 0; j < numevents; j++) {
				aeFileEvent *fe = &eventLoop->events[eventLoop->fired[j].fd];
				int mask = eventLoop->fired[j].mask;
				int fd = eventLoop->fired[j].fd;
				int rfired = 0;
				
				if(fe->mask & mask & AE_READABLE) {
						rfired = 1;
						fe->rfileProc(eventLoop, fd, fe->clientData, mask);
				}

				if(fe->mask & mask & AE_WRITABLE) {
						if(!rfired || fe->wfileProc != fe->rfileProc) {
								fe->wfileProc(eventLoop, fd, fe->clientData, mask);
						}
				}
				processed++;
		}

		return processed;
}

void aeMain(aeEventLoop *eventLoop) {
		eventLoop->stop = 0;
		while(!eventLoop->stop) {
				aeProcessEvents(eventLoop, AE_ALL_EVENTS);
		}
}
