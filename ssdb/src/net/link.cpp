#include "link.h"
#include "../serv.h"
#include "../include.h"
#include <assert.h>

#define INIT_BUFFER_SIZE 1024
#define BEST_BUFFER_SIZE (8 * 1024)
#define MAX_PACKET_SIZE (128 * 1024 * 1024)

Link::Link() {
    input = new Buffer(INIT_BUFFER_SIZE);
    output = new Buffer(INIT_BUFFER_SIZE);
    client = (uv_stream_t *) malloc(sizeof(uv_tcp_t));
    redis = new RedisLink();
    recv_bytes.clear();

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
    if(client) {
        uv_close((uv_handle_t*) client, NULL);
    }
}

void Link::onRead(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {
    Link *link = (Link *)client->data;
    NetworkServer *net = (NetworkServer *)link->data;
    write_req_t *resp;

    if(nread <= 0) {
        return;
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

    // init output pointer
    resp = (write_req_t *) malloc(sizeof(write_req_t));
    resp->data = link;
    resp->nread = link->output->size();
    resp->buf = uv_buf_init(link->output->data(), link->output->size());

    if (uv_write(&resp->req, client, &resp->buf, 1, Link::afterWrite)) {
        log_debug("uv_write failed");
    }
    return;
error:
    free(buf->base);
    if(net) {
        net->link_count--;
    }
    if(link)
        delete link;
}

const std::vector<Bytes>* Link::recv_req() {
    log_debug("input size = %d", input->size());
    recv_bytes.clear();
    if(input->size() <= 0) {
        return &recv_bytes;
    }
	// TODO: 记住上回的解析状态
	int parsed = 0;
	int size = input->size();
	char *head = input->data();

	// ignore leading empty lines
	while(size > 0 && (head[0] == '\n' || head[0] == '\r')){
		head ++;
		size --;
		parsed ++;
	}
	
	// Redis protocol supports
	if(head[0] == '*'){
		const std::vector<Bytes> *req = redis->recv_req(input);
        if(req != NULL) {
            recv_bytes = *req;
		}
        return req;
	}

	while(size > 0){
		char *body = (char *)memchr(head, '\n', size);
		if(body == NULL){
			break;
		}
		body ++;

		int head_len = body - head;
		if(head_len == 1 || (head_len == 2 && head[0] == '\r')){
			// packet end
			parsed += head_len;
			input->decr(parsed);
			return &recv_bytes;;
		}
		if(head[0] < '0' || head[0] > '9'){
			//log_warn("bad format");
			return NULL;
		}

		char head_str[20];
		if(head_len > (int)sizeof(head_str) - 1){
			return NULL;
		}
		memcpy(head_str, head, head_len - 1); // no '\n'
		head_str[head_len - 1] = '\0';

		int body_len = atoi(head_str);
		if(body_len < 0){
			//log_warn("bad format");
			return NULL;
		}
		//log_debug("size: %d, head_len: %d, body_len: %d", size, head_len, body_len);
		size -= head_len + body_len;
		if(size < 0){
			break;
		}

		recv_bytes.push_back(Bytes(body, body_len));

		head += head_len + body_len;
		parsed += head_len + body_len;
		if(size >= 1 && head[0] == '\n'){
			head += 1;
			size -= 1;
			parsed += 1;
		}else if(size >= 2 && head[0] == '\r' && head[1] == '\n'){
			head += 2;
			size -= 2;
			parsed += 2;
		}else if(size >= 2){
			// bad format
			return NULL;
		}else{
			break;
		}
		if(parsed > MAX_PACKET_SIZE){
			 return NULL;
		}
	}

	if(input->space() == 0){
		input->nice();
		if(input->space() == 0){
			if(input->grow() == -1){
				return NULL;
			}
		}
	}

    recv_bytes.clear();
	// not ready
    return &recv_bytes;
}

void Link::afterWrite(uv_write_t* req, int status) {
    write_req_t* resp;
    resp = (write_req_t*) req;

    int nread = resp->nread;
    Link* link = (Link *)resp->data;
    log_debug("after write %d\n", nread);

	link->output->decr(nread);
    assert(link != NULL);

    free(req);
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

int Link::send(const Bytes &s1, const Bytes &s2) {
    output->append_record(s1);
    output->append_record(s2);
    output->append('\n');

    // init output pointer
    write_req_t *resp = (write_req_t *) malloc(sizeof(write_req_t));
    resp->data = this;
    resp->nread = output->size();
    resp->buf = uv_buf_init(output->data(), output->size());
    uv_write(&resp->req, client, &resp->buf, 1, Link::afterWrite);
    return 0;
}

int Link::send(const Bytes &s1, const Bytes &s2, const Bytes &s3, const Bytes &s4){
    output->append_record(s1);
    output->append_record(s2);
    output->append_record(s3);
    output->append_record(s4);
    output->append('\n');

    write_req_t *resp = (write_req_t *) malloc(sizeof(write_req_t));
    resp->data = this;
    resp->nread = output->size();
    resp->buf = uv_buf_init(output->data(), output->size());
    uv_write(&resp->req, client, &resp->buf, 1, Link::afterWrite);
    return 0;
}

int Link::send(const Bytes &s1){
    output->append_record(s1);
    output->append('\n');
    write_req_t *resp = (write_req_t *) malloc(sizeof(write_req_t));
    resp->data = this;
    resp->nread = output->size();
    resp->buf = uv_buf_init(output->data(), output->size());
    uv_write(&resp->req, client, &resp->buf, 1, Link::afterWrite);
    return 0;
}

