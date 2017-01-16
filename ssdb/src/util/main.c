#include "redis_util.h"

int main() {
    char *data = "*3\r\n$3\r\nSET\r\n$5\r\nmykey\r\n$7\r\nmyvalue\r\n";
    Buffer *buf = new Buffer(100);
    buf->append(data);
    parser(buf);
    return 0;
}
