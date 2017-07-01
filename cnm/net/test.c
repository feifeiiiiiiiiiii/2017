#include "ae.h"
#include "anet.h"
#include <stdio.h>

static char neterr[ANET_ERR_LEN];
static aeEventLoop *eventLoop;
		struct timeval tv;

int
loop_forver_time_cb(aeEventLoop *eventLoop, void *arg)
{
	printf("forver timer\n");
	return 1;
}

int
loop_once_time_cb(aeEventLoop *eventLoop, void *arg)
{
	printf("loop_once\n");
	return -1;
}

void acceptTcpHandler(aeEventLoop *el, int fd, void *privdata, int mask) {
		printf("acceptTcpHandler\n");

		int cfd, cport, max = 100;
		char cip[100];

		while(max--) {
				cfd = anetTcpAccept(neterr, fd, cip, sizeof(cip), &cport);
				if(cfd == ANET_ERR) {
						return;
				}
				printf("accept success %s %d\n", cip, cport);
		}
}

int main() {
		int fd;

		eventLoop = aeCreateEventLoop(1024);

		fd = anetTcpServer(neterr, 6379, "0.0.0.0", 128);
		if(fd == ANET_ERR) {
				printf("anetTcpServer error %s\n", neterr);
				return 0;
		}
		anetNonBlock(NULL,fd);

		if(aeCreateFileEvent(eventLoop, fd, AE_READABLE, acceptTcpHandler, NULL) == AE_ERR) {
				printf("aeCreateFileEvent failed\n");
				return 0;
		}
		
	tv.tv_sec = random() % 10;
	tv.tv_usec = random() % 5000L;
	aeCreateTimeEvent(eventLoop, &tv, loop_forver_time_cb, NULL, 0);
	aeCreateTimeEvent(eventLoop, &tv, loop_once_time_cb, NULL, 0);
		aeMain(eventLoop);
		return 0;
}
