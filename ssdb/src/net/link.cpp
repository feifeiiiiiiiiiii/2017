#include "link.h"
#include "../serv.h"
#include "../include.h"
#include <assert.h>

#define INIT_BUFFER_SIZE 1024
#define BEST_BUFFER_SIZE (8 * 1024)

Link::Link() {
    input = new Buffer(INIT_BUFFER_SIZE);
    output = new Buffer(INIT_BUFFER_SIZE);
    client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    redis = new RedisLink();
    recv_bytes.clear();

    // init output pointer
    resp = (write_req_t *) malloc(sizeof(write_req_t));
    resp->data = this;
    create_time = millitime();
    active_time = create_time;
}

Link::~Link() {
    if(input) {
        delete input;
    }
    if(output) {
        delete output;
    }
    if(redis) {
        delete redis;
    }
    if(resp) {
        free(resp);
    }
    if(client) {
        uv_close((uv_handle_t*) client, NULL);
    }
}

void Link::onRead(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {
    Link *link = (Link *)client->data;
    NetworkServer *net = (NetworkServer *)link->data;

    if(nread <= 0) {
        goto error;
    }
    // copy uv buffer to Link buffer
    link->copy2Buffer(buf->base, nread);

    while(true) {
        // try parse redis command
        const std::vector<Bytes> *req = link->recv_req();

        if(req == NULL) {
            log_debug("recv_req error");
            goto error;
        }
        if(req->empty()) {
            break;
        }
        // handle req

        link->active_time = millitime();

        ProcJob *job = new ProcJob();
        job->link = link;
        job->serv = net;
        job->req = req;

        link->proc(job);
        free(job);
    }

    free(buf->base);
    if(link->output->empty()) return;

    // init response
    link->resp->nread = link->output->size();
    link->resp->buf = uv_buf_init(link->output->data(), link->output->size());

    if (uv_write(&link->resp->req, client, &link->resp->buf, 1, Link::afterWrite)) {
        log_debug("uv_write failed");
    }
    return;
error:
    free(buf->base);
    if(net) {
        net->link_count--;
    }
    delete link;
}

const std::vector<Bytes>* Link::recv_req() {
    recv_bytes.clear();
    if(input->size() <= 0) {
        return &recv_bytes;
    }
    return redis->recv_req(input);
}

void Link::afterWrite(uv_write_t* req, int status) {
    write_req_t* resp;
    resp = (write_req_t*) req;

    int nread = resp->nread;
    Link* link = (Link *)resp->data;
	link->output->decr(nread);
    assert(link != NULL);

    if (status == 0) {
        return;
    }

    log_debug("uv_write error: %s - %s\n", uv_err_name(status), uv_strerror(status));
}

int Link::proc(ProcJob *job) {
    job->result = PROC_OK;
    job->stime = millitime();
    NetworkServer *net = job->serv;

    const std::vector<Bytes> *req = job->req;
    do {
        Command *cmd = net->proc_map.get_proc(req->at(0));
        if(!cmd){
            job->resp.push_back("client_error");
            job->resp.push_back("Unknown Command: " + req->at(0).String());
            break;
        }
        job->cmd = cmd; 

        proc_t p = cmd->proc;
        job->time_wait = 1000 * (millitime() - job->stime);
        job->result = (*p)(net, job->link, *req, &job->resp);
        job->time_proc = 1000 * (millitime() - job->stime) - job->time_wait;
    } while(0);

	return job->link->append2buffer(job->resp.resp);
}

int Link::append2buffer(const std::vector<std::string> &resp) {
    return this->redis->send_resp(this->output, resp);
}
