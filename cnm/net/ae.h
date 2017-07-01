#ifndef __AE_H_
#define __AE_H_

#define AE_OK 0
#define AE_ERR -1

#define AE_NONE 0
#define AE_READABLE 1
#define AE_WRITABLE 2

#define AE_FILE_EVENTS 1
#define AE_TIME_EVENTS 2
#define AE_ALL_EVENTS (AE_FILE_EVENTS|AE_TIME_EVENTS)
#define AE_DONT_WAIT 4

#define AE_NOMORE -1

#include <sys/time.h>
#include "tree.h"

struct aeEventLoop;

typedef void aeFileProc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
typedef int  aeTimeProc(struct aeEventLoop *eventLoop, void *clientData);

typedef struct aeFileEvent {
    int mask; /* one of AE_(READABLE|WRITEABLE) */
    aeFileProc *rfileProc;
    aeFileProc *wfileProc;
    void *clientData;
} aeFileEvent;

typedef struct aeFireEvent {
		int fd;
		int mask;
} aeFireEvent;

typedef struct aeTimeEvent {
		struct timeval timeout;
		struct timeval tv;
		aeTimeProc *timeProc;
		int mask;
		void *clientData;
		RB_ENTRY(aeTimeEvent) timeNode;
} aeTimeEvent;

typedef struct aeEventLoop {
    int maxfd;
    int setsize;
		struct timeval eventtv;
    aeFileEvent *events; /* fd as idx */
		aeFireEvent *fired; /* active events */
		aeTimeEvent timeEvent;
		RB_HEAD(eventTree, aeTimeEvent) timetree;
    int stop;
		void *apidata; /* ptr to epoll state */
} aeEventLoop;

aeEventLoop *aeCreateEventLoop(int setsize);
void aeDeleteEventLoop(aeEventLoop *eventLoop);
void aeStop(aeEventLoop *eventLoop);
int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask, aeFileProc *proc, void *clientData);
int aeProcessEvents(aeEventLoop *eventLoop, int flags);
void aeMain(aeEventLoop *eventLoop);
int aeCreateTimeEvent(aeEventLoop *eventLoop, struct timeval *tv, aeTimeProc *proc, void *clientData, int mask);
void aeProcessTimeEvents(aeEventLoop *eventLoop);

#endif /* __AE_H_ */
