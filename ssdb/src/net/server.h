#ifndef NET_SERVER_H_
#define NET_SERVER_H_

#include <uv.h>
#include <vector>

#include "link.h"
#include "proc.h"
#include "../util/config.h"

class NetworkServer {
private:
    NetworkServer();
    uv_tcp_t tcp_server;
    uv_loop_t* loop;

    static void acceptLink(uv_stream_t *server, int status);
    Link* newLink();

public:
    ~NetworkServer();
    static NetworkServer* init(const Config *cf);
    void serve();
    void *data;
    int link_count;
    ProcMap proc_map;
};

#endif
