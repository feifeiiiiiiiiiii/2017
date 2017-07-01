#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void sys_read(int fd, char *buf, size_t len) {
  ssize_t ret;

  while(len != 0 && (ret = read(fd, buf, len)) != 0) {
    if(ret == -1) {
      if(errno == EINTR) {
        continue;
      }
      break;
    }
    buf += len;
    len -= ret;
  }
}

int main() {
  int fd = open("read.c", O_RDONLY);
  if(fd == -1) {
    printf("open file failed\n");
    return 0;
  }

  char buf[1024];
  sys_read(fd, buf, 1000);
  printf("%s\n", buf);

  close(fd);
}
