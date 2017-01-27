/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#ifndef SSDB_SLAVE_H_
#define SSDB_SLAVE_H_

#include <stdint.h>
#include <string>
#include <pthread.h>
#include <vector>
#include <uv.h>
#include "ssdb/ssdb_impl.h"
#include "ssdb/binlog.h"
#include "util/bytes.h"
#include "net/link.h"

class Slave {
private:
    static const int DISCONNECTED = 0;
    static const int INIT = 1;
    static const int COPY = 2;
    static const int SYNC = 4;
    static const int OUT_OF_SYNC = 8;
    int status;
    bool is_mirror;
    char log_type;
    uint64_t copy_count;
    uint64_t sync_count;

	uint64_t last_seq;
	std::string last_key;
		
	std::string id_;
 	uv_thread_t tid;
	uv_loop_t *loop;
	uv_connect_t connect_req;
	bool thread_quit;

    Link *link;
	SSDB *ssdb;
	SSDB *meta;
    void migrate_old_status();
	std::string status_key();
	void load_status();
	void save_status();
	static void _run_thread(void *arg);
    static void onRead(uv_stream_t* tcp, ssize_t nread, const uv_buf_t* buf);
    static void allocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
        buf->base = (char *)malloc(suggested_size);
        buf->len = suggested_size;
    }
    int proc(const std::vector<Bytes> &req);
    int proc_sync(const Binlog &log, const std::vector<Bytes> &req);
	int proc_copy(const Binlog &log, const std::vector<Bytes> &req);
public:
	Slave(SSDB *ssdb, SSDB *meta, const std::string ip, int port);
	~Slave();
	void start();
	void stop();
	static void on_connect(uv_connect_t *req, int status);
    static void afterWrite(uv_write_t* req, int status);
	void set_id(const std::string &id);
	std::string master_ip;
    int master_port;
    write_req_t *resp;
};

#endif
