#include "server.h"
#include <stdlib.h>

NetworkServer::NetworkServer() {
    link_count = 0;
}

NetworkServer::~NetworkServer() {
}

NetworkServer* NetworkServer::init(const Config *cf) {
    int r;
    struct sockaddr_in addr;

    NetworkServer *serv = new NetworkServer();
    if(serv == NULL) {
        fprintf(stderr, "Alloc NetworkServer Instance error\n");
        return NULL;
    }

    serv->loop = uv_default_loop();

    // TODO config
    r = uv_ip4_addr(cf->ip.c_str(), cf->port, &addr);
    if(r) {
        fprintf(stderr, "uv_ip4_addr error\n");
        delete serv;
        return NULL;
    }

    r = uv_tcp_init(serv->loop, &serv->tcp_server);
    if(r) {
        fprintf(stderr, "uv_tcp_init error\n");
        delete serv;
        return NULL;
    }

    r = uv_tcp_bind(&serv->tcp_server, (const struct sockaddr*)&addr, 0);
    if(r) {
        fprintf(stderr, "uv_tcp_bind error\n");
        delete serv;
        return NULL;
    }

    r = uv_listen((uv_stream_t*)&serv->tcp_server, SOMAXCONN, NetworkServer::acceptLink);
    if(r) {
        fprintf(stderr, "uv_listen error\n");
        delete serv;
        return NULL;
    }

    serv->tcp_server.data = serv;
    return serv;
}

void NetworkServer::acceptLink(uv_stream_t *server, int status) {
    uv_stream_t* stream;
    int r;

    if (status != 0) {
        fprintf(stderr, "Connect error %s\n", uv_err_name(status));
        return;
    }

    NetworkServer *serv = (NetworkServer *)server->data;
    Link *link = serv->newLink();
    
    uv_tcp_init(serv->loop, link->client);
    if (uv_accept(server, (uv_stream_t*) link->client) == 0) {
        serv->link_count++;
        uv_read_start((uv_stream_t*) link->client, Link::allocBuffer, Link::onRead);
    }
    else {
        printf("direct remove link\n");
        delete link;
    }
}

void NetworkServer::serve() {
    uv_run(loop, UV_RUN_DEFAULT);
}

Link* NetworkServer::newLink() {
    Link *link = new Link();
    link->client->data = link;
    link->data = this;
    return link;
}
