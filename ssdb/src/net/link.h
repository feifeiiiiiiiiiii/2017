#ifndef NET_LINK_H_
#define NET_LINK_H_

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <uv.h>
#include <vector>

#include "../util/bytes.h"
#include "../util/log.h"
#include "link_redis.h"
#include "proc.h"

typedef struct {
    uv_write_t req;
    uv_buf_t buf;
    int nread;
    void *data;
} write_req_t;

class Link {
private:
    Buffer *input;
    Buffer *output;

	std::vector<Bytes> recv_bytes;
    RedisLink *redis;


public:
    uv_tcp_t *client;
    void *data;
    write_req_t *resp;

    double create_time;
    double active_time;

    Link();
    ~Link();

    int fd() const {
        return client->u.fd;
    }

    int append2buffer(const std::vector<std::string> &resp);

    static void allocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
        buf->base = (char *)malloc(suggested_size);
        buf->len = suggested_size;
    }

    static void onRead(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf);

    int proc(ProcJob *job);

    static void afterWrite(uv_write_t* req, int status);

    void copy2Buffer(const char *data, ssize_t nread) {
        input->append(data, (int)nread);
    }

    const std::vector<Bytes> *recv_req();
};

#endif
