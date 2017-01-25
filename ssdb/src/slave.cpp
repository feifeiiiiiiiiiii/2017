/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "util/log.h"
#include "net/link.h"
#include "slave.h"
#include "include.h"

#define INIT_BUFFER_SIZE 1024
#define BEST_BUFFER_SIZE (8 * 1024)

Slave::Slave(SSDB *ssdb, SSDB *meta, const std::string ip, int port){
	thread_quit = false;
	this->ssdb = ssdb;
	this->meta = meta;
	this->master_ip = ip;
	this->master_port = port;

    this->input = new Buffer(INIT_BUFFER_SIZE);
    this->output = new Buffer(INIT_BUFFER_SIZE);
	
	{
		char buf[128];
		snprintf(buf, sizeof(buf), "%s|%d", master_ip.c_str(), master_port);
		this->set_id(buf);
	}
	
	this->last_seq = 0;
	this->last_key = "";
}

Slave::~Slave(){
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

    slave->loop = (uv_loop_t *)malloc(sizeof *slave->loop);
	uv_loop_init(slave->loop);
	slave->connect_req.data = slave;

	uv_tcp_init(slave->loop, &slave->client);

	struct sockaddr_in req_addr;
    fprintf(stderr, "%s %d\n", slave->master_ip.c_str(), slave->master_port);
    int ret = uv_ip4_addr(slave->master_ip.c_str(), slave->master_port, &req_addr);
    if(ret != 0) {
	    fprintf(stderr, "error on_write_end\n");
        return;
    }
	
	uv_tcp_connect(&slave->connect_req, &slave->client, 
                    (const struct sockaddr *)&req_addr, Slave::on_connect);
    uv_run(slave->loop, UV_RUN_DEFAULT);
}

void Slave::on_connect(uv_connect_t *req, int status) {
	if (status == -1) {
		fprintf(stderr, "error on_write_end");
		return;
	}
    Slave *slave = (Slave *)req->data;
    uv_write_t write_req;
    uv_buf_t buf;

    // 
    slave->send("sync140", str(slave->last_seq), "sync");
    // init response
    buf = uv_buf_init(slave->output->data(), slave->output->size());

    uv_stream_t* stream = req->handle;
    uv_write(&write_req, stream, &buf, 1, NULL);
    uv_read_start(stream, Slave::allocBuffer, Slave::onRead);
}

void Slave::onRead(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf) {
    if(nread <= 0) {
        printf("onRead = %ld\n", nread);
    }
    printf("on read %ld %s\n", nread, buf->base);
}

int Slave::send(const Bytes &s1, const Bytes &s2, const Bytes &s3){
    output->append("*3\r\n");

    output->append("$");
    output->append(str(s1.size()));
    output->append("\r\n");
    output->append(s1);
    output->append("\r\n");

    output->append("$");
    output->append(str(s2.size()));
    output->append("\r\n");
    output->append(s2);
    output->append("\r\n");

    output->append("$");
    output->append(str(s3.size()));
    output->append("\r\n");
    output->append(s3);
    output->append("\r\n");
    return 0;
}
