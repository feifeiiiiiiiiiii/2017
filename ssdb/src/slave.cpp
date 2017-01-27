/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "util/log.h"
#include "slave.h"
#include "include.h"

#define INIT_BUFFER_SIZE 1024
#define BEST_BUFFER_SIZE (8 * 1024)

static inline
std::string encode_kv_key(const Bytes &key){
	std::string buf;
	buf.append(1, DataType::KV);
	buf.append(key.data(), key.size());
	return buf;
}

static inline
int decode_kv_key(const Bytes &slice, std::string *key){
	Decoder decoder(slice.data(), slice.size());
	if(decoder.skip(1) == -1){
		return -1;
	}
	if(decoder.read_data(key) == -1){
		return -1;
	}
	return 0;
}

Slave::Slave(SSDB *ssdb, SSDB *meta, const std::string ip, int port){
	thread_quit = false;
	this->ssdb = ssdb;
	this->meta = meta;
	this->master_ip = ip;
	this->master_port = port;


	{
		char buf[128];
		snprintf(buf, sizeof(buf), "%s|%d", master_ip.c_str(), master_port);
		this->set_id(buf);
	}
	
	this->last_seq = 0;
	this->last_key = "";
	
	this->copy_count = 0;
	this->sync_count = 0;
}

Slave::~Slave(){
    if(link) {
        free(link);
    }
}

void Slave::start(){
    if(master_ip.empty()) return;
    if(master_port <= 0) return;

    migrate_old_status();
    load_status();
    log_debug("last_seq: %" PRIu64 ", last_key: %s",
              last_seq, hexmem(last_key.data(), last_key.size()).c_str());

    // create pthread to connect master server
	int r = uv_thread_create(&tid, Slave::_run_thread, (void *)this);
	if(r != 0) {
		log_error("can't create thread: %s", strerror(r));
	}
}

void Slave::stop(){
    if(master_ip.empty()) return;
    if(master_port <= 0) return;
	thread_quit = true;
}

void Slave::set_id(const std::string &id) {
	this->id_ = id;
}

void Slave::migrate_old_status() {
}

void Slave::load_status(){
}

void Slave::save_status(){
}

std::string Slave::status_key() {
	std::string key;
	key = "slave.status." + this->id_;
	return key;
}

void Slave::_run_thread(void *arg) {
	Slave *slave = (Slave *)arg;
    slave->link = new Link();
    slave->link->client->data = slave->link;
	slave->link->data = slave;

    slave->loop = uv_loop_new();
	slave->connect_req.data = slave;

	uv_tcp_init(slave->loop, (uv_tcp_t *)slave->link->client);

	struct sockaddr_in req_addr;
    int ret = uv_ip4_addr(slave->master_ip.c_str(), slave->master_port, &req_addr);
    if(ret != 0) {
	    fprintf(stderr, "error on_write_end\n");
        return;
    }
	
	uv_tcp_connect(&slave->connect_req, (uv_tcp_t *)slave->link->client, 
                    (const struct sockaddr *)&req_addr, Slave::on_connect);
    uv_run(slave->loop, UV_RUN_DEFAULT);
	pthread_exit(NULL);
}

void Slave::on_connect(uv_connect_t *req, int status) {
	if (status == -1) {
		fprintf(stderr, "error on_write_end");
		return;
	}
    Slave *slave = (Slave *)req->data;
    slave->link->send("sync140", str(slave->last_seq), slave->last_key, "sync");

    uv_read_start((uv_stream_t *)slave->link->client, Slave::allocBuffer, Slave::onRead);
}

void Slave::onRead(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf) {
    if(nread <= 0) {
        printf("onRead = %ld\n", nread);
		return;
    }
    Link *link = (Link *)tcp->data;
    assert(link != NULL);
    link->copy2Buffer(buf->base, nread);
	Slave *slave = (Slave *)link->data;
	const std::vector<Bytes> *req;

    while(true) {
        // try parse redis command
        req = link->recv_req();
        if(req == NULL) {
			log_debug("recv_req error");
            goto error;
        }
        if(req->empty()) {
			log_debug("need more data");
			log_debug("---------------\n");
            break;
        }
		log_debug("need proc");
		slave->proc(*req);
		log_debug("---------------\n");
    }
    free(buf->base);
    return;
error:
	free(buf->base);
    delete link;
}

int Slave::proc(const std::vector<Bytes> &req){
	Binlog log;
	if(log.load(req[0]) == -1){
		log_error("invalid binlog!");
		return 0;
	}
	const char *sync_type = this->is_mirror? "mirror" : "sync";
	switch(log.type()){
		case BinlogType::NOOP:
			//return this->proc_noop(log, req);
			break;
		case BinlogType::CTRL:
			if(log.key() == "OUT_OF_SYNC"){
				status = OUT_OF_SYNC;
				log_error("OUT_OF_SYNC, you must reset this node manually!");
			}
			break;
		case BinlogType::COPY:{
			status = COPY;
			if(req.size() >= 2){
				log_debug("[%s] [%d]", sync_type, req[1].size());
			}else{
				log_debug("[%s]", sync_type);
			}
			this->proc_copy(log, req);
			break;
		}
		case BinlogType::SYNC:
		case BinlogType::MIRROR:{
			status = SYNC;
			if(++sync_count % 1000 == 1){
				log_info("sync_count: %" PRIu64 ", last_seq: %" PRIu64 ", seq: %" PRIu64 "",
					sync_count, this->last_seq, log.seq());
			}
			if(req.size() >= 2){
				log_debug("[%s] [%d]", sync_type, req[1].size());
			}else{
				log_debug("[%s]", sync_type);
			}
			this->proc_sync(log, req);
			break;
		}
		default:
			break;
	}
	return 0;
}

int Slave::proc_sync(const Binlog &log, const std::vector<Bytes> &req){
	switch(log.cmd()) {
		case BinlogCommand::KSET:
			{
				if(req.size() != 2){
					break;
				}
				std::string key;
				if(decode_kv_key(log.key(), &key) == -1){
					break;
				}
				log_debug("set %s", hexmem(key.data(), key.size()).c_str());
				if(ssdb->set(key, req[1]) == -1){
					return -1;
				}
			}
			break;
		case BinlogCommand::KDEL:
			{
				std::string key;
				if(decode_kv_key(log.key(), &key) == -1){
					break;
				}
				log_debug("del %s", hexmem(key.data(), key.size()).c_str());
				if(ssdb->del(key) == -1){
					return -1;
				}
			}
			break;
		default:
			log_error("unknown binlog, type=%d, cmd=%d", log.type(), log.cmd());
			break;
	}
	this->last_seq = log.seq();
	if(log.type() == BinlogType::COPY){
		this->last_key = log.key().String();
	}
	this->save_status();
    return 0;
}

int Slave::proc_copy(const Binlog &log, const std::vector<Bytes> &req){
	switch(log.cmd()){
		case BinlogCommand::BEGIN:
			log_info("copy begin");
			break;
		case BinlogCommand::END:
			log_info("copy end, copy_count: %" PRIu64 ", last_seq: %" PRIu64 ", seq: %" PRIu64,
				copy_count, this->last_seq, log.seq());
			this->status = SYNC;
			this->last_key = "";
			this->save_status();
			break;
		default:
			if(++copy_count % 1000 == 1){
				log_info("copy_count: %" PRIu64 ", last_seq: %" PRIu64 ", seq: %" PRIu64 "",
					copy_count, this->last_seq, log.seq());
			}
			return proc_sync(log, req);
			break;
	}
	return 0;
}
