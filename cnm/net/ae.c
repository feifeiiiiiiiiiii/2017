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

static int
compare(aeTimeEvent *a, aeTimeEvent *b) {
		if (timercmp(&a->timeout, &b->timeout, <))
				return (-1);
		else if (timercmp(&a->timeout, &b->timeout, >))
				return (1);
		if (a < b)
				return (-1);
		else if (a > b)
				return (1);
		return (0);
}

RB_PROTOTYPE(eventTree, aeTimeEvent, timeNode, compare);
RB_GENERATE(eventTree, aeTimeEvent, timeNode, compare);

void
timeout_correct(aeEventLoop *eventLoop, struct timeval *off)
{
		struct aeTimeEvent *ev;
		RB_FOREACH(ev, eventTree, &eventLoop->timetree)
		timersub(&ev->timeout, off, &ev->timeout);
}

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
		gettimeofday(&eventLoop->eventtv, NULL);
		RB_INIT(&eventLoop->timetree);
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
		struct timeval tv = {0, 5};

		if(eventLoop->maxfd == -1) return 0;

		numevents = aeApiPoll(eventLoop, &tv);

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
				aeProcessTimeEvents(eventLoop);
		}
}

int aeCreateTimeEvent(aeEventLoop *eventLoop, struct timeval *tv, aeTimeProc *proc, void *clientData, int mask) {
		if(tv == NULL) return AE_ERR;
		aeTimeEvent *te;
		te = malloc(sizeof(*te));
		if(te == NULL) return AE_ERR;
		te->timeProc = proc;
		te->mask = mask;

		struct timeval now;

		gettimeofday(&now, NULL);
		timeradd(&now, tv, &te->timeout);
		RB_INSERT(eventTree, &eventLoop->timetree, te);
		return AE_OK;
}

void aeProcessTimeEvents(aeEventLoop *eventLoop) {
		struct timeval now;
		aeTimeEvent *te, *next;
		gettimeofday(&now, NULL);
		
		for(te = RB_MIN(eventTree, &eventLoop->timetree); te; te = next) {
				if(timercmp(&te->timeout, &now, >)) {
						break;
				}

				next = RB_NEXT(eventTree, &eventLoop->timetree, te);
				te->timeProc(eventLoop, te->clientData);
				RB_REMOVE(eventTree, &eventLoop->timetree, te);
		}
}
