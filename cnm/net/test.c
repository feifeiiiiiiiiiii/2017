#include "ae.h"
#include "anet.h"
#include <stdio.h>

static char neterr[ANET_ERR_LEN];
static aeEventLoop *eventLoop;

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

		fd = anetTcpServer(neterr, 6379, "127.0.0.1", 128);
		if(fd == ANET_ERR) {
				printf("anetTcpServer error %s\n", neterr);
				return 0;
		}
		anetNonBlock(NULL,fd);

		if(aeCreateFileEvent(eventLoop, fd, AE_READABLE, acceptTcpHandler, NULL) == AE_ERR) {
				printf("aeCreateFileEvent failed\n");
				return 0;
		}

		aeMain(eventLoop);
		return 0;
}
