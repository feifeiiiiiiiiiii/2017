/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include <assert.h>
#include <errno.h>
#include <string>
#include "backend_sync.h"
#include "util/log.h"
#include "util/strings.h"

BackendSync::BackendSync(SSDBImpl *ssdb){
	this->ssdb = ssdb;
}

BackendSync::~BackendSync(){
}

void BackendSync::proc(const Link *link){
    log_info("accept sync client");
    struct run_arg *arg = new run_arg();
    arg->link = link;
    arg->backend = this;

 	uv_thread_t tid;
	int r = uv_thread_create(&tid, BackendSync::_run_thread, (void *)arg);
    if(r != 0){
        log_error("can't create thread: %s", strerror(r));
        delete link;
    }
}

void BackendSync::_run_thread(void *arg) {
    log_debug("BackendSync thread run");
    struct run_arg *p = (struct run_arg*)arg;
    BackendSync *backend = (BackendSync *)p->backend;
    Link *link = (Link *)p->link;
    delete p;

    SSDBImpl *ssdb = (SSDBImpl *)backend->ssdb;
	BinlogQueue *logs = ssdb->binlogs;

	Client client(backend);
	client.link = link;
	client.init();
	
	{
		pthread_t tid = pthread_self();
		Locking l(&backend->mutex);
		backend->workers[tid] = &client;
	}

	while(true) {
		bool is_empty = true;
		if(client.status == Client::OUT_OF_SYNC){
		}else{
			// WARN: MUST do first sync() before first copy(), because
			// sync() will refresh last_seq, and copy() will not
			//if(client.sync(logs)){ // sync seq or binlog
			//	is_empty = false;
			//}
			if(client.status == Client::COPY){
				if(client.copy()){
					is_empty = false;
				}
			}
			if(is_empty) {
				usleep(300 * 1000);
			}
		}
	}
}

/* Client */

BackendSync::Client::Client(const BackendSync *backend){
	status = Client::INIT;
	this->backend = backend;
	link = NULL;
	last_seq = 0;
	last_noop_seq = 0;
	last_key = "";
	is_mirror = false;
}

BackendSync::Client::~Client() {}

void BackendSync::Client::init() {
	const std::vector<Bytes> *req = this->link->last_recv();
	last_seq = 0;
	if(req->size() > 1){
		last_seq = req->at(1).Uint64();
	}
	if(req->size() > 2){
        last_key = req->at(2).String();
    }
	// is_mirror
	if(req->size() > 3){
		if(req->at(3).String() == "mirror"){
			is_mirror = true;
		}
	}

	const char *type = is_mirror? "mirror" : "sync";

    if(last_key == "" && last_seq == 0) {
        log_info("[%s]:, copy begin, seq: %" PRIu64 ", key: '%s'",
                 type,
                 last_seq, hexmem(last_key.data(), last_key.size()).c_str()
                );
        this->reset();
    }
}

void BackendSync::Client::reset() {
	this->status = Client::COPY;
	this->last_seq = 0;
	this->last_key = "";

	Binlog log(this->last_seq, BinlogType::COPY, BinlogCommand::BEGIN, "");
	link->send(log.repr(), "copy_begin");
}

void BackendSync::Client::noop() {
	uint64_t seq;
	if(this->status == Client::COPY && this->last_key.empty()){
		seq = 0;
	}else{
		seq = this->last_seq;
		this->last_noop_seq = this->last_seq;
	}
	Binlog noop(seq, BinlogType::NOOP, BinlogCommand::NONE, "");
	link->send(noop.repr(), "noop");
}

int BackendSync::Client::copy() {
	if(this->iter == NULL){
		std::string key = this->last_key;
		if(this->last_key.empty()){
			key.push_back(DataType::MIN_PREFIX);
		}
		this->iter = backend->ssdb->iterator(key, "", -1);
	}
	int ret = 0;
	int iterate_count = 0;
	int64_t stime = time_ms();
	while(true){
		// Prevent copy() from blocking too long
		if(++iterate_count > 1000){
			break;
		}
		
		if(!iter->next()){
			goto copy_end;
		}
		Bytes key = iter->key();
		if(key.size() == 0){
			continue;
		}
		// finish copying all valid data types
		if(key.data()[0] > DataType::MAX_PREFIX){
			goto copy_end;
		}
		Bytes val = iter->val();
		this->last_key = key.String();
			
		char cmd = 0;
		char data_type = key.data()[0];
		if(data_type == DataType::KV){
			cmd = BinlogCommand::KSET;
		}else{
			continue;
		}
		ret++;
		log_debug("last_seq = %lld, cmd = %d, key = %s", this->last_seq, cmd, key.String().c_str());
		Binlog log(this->last_seq, BinlogType::COPY, cmd, slice(key));
		link->send(log.repr(), val);
	}
	return ret;

copy_end:		
	log_info("copy end");
	this->status = Client::SYNC;
	delete this->iter;
	this->iter = NULL;

	Binlog log(this->last_seq, BinlogType::COPY, BinlogCommand::END, "");
	link->send(log.repr(), "copy_end");
	return 1;
}

int BackendSync::Client::sync(BinlogQueue *logs) {
	Binlog log;
	while(1) {
		int ret = 0;
		uint64_t expect_seq = this->last_seq + 1;
		if(this->status == Client::COPY && this->last_seq == 0){
			ret = logs->find_last(&log);
		}else{
			ret = logs->find_next(expect_seq, &log);
		}
		if(ret == 0){
			return 0;
		}
		log_debug("expect_seq %lld, ret = %d, status = %d", expect_seq, ret, log.seq(), this->status);
		if(this->status == Client::COPY && log.key() > this->last_key){
			log_debug("last_key: '%s', drop",
			 hexmem(this->last_key.data(), this->last_key.size()).c_str());
            this->last_seq = log.seq();
            if(this->iter){
                delete this->iter;
                this->iter = NULL;
            }
			continue;
		}
		if(this->last_seq != 0 && log.seq() != expect_seq){
			log_warn("OUT_OF_SYNC!");
			this->out_of_sync();
			return 1;
		}
		// update last_seq
		this->last_seq = log.seq();

		char type = log.type();
		if(type == BinlogType::MIRROR && this->is_mirror){
			if(this->last_seq - this->last_noop_seq >= 1000){
				this->noop();
				return 1;
			}else{
				continue;
			}
		}

		break;
	}
	log_debug("cmd = %d", log.cmd());
	int ret = 0;
	std::string val;
	switch(log.cmd()){
		case BinlogCommand::KSET:
			ret = backend->ssdb->raw_get(log.key(), &val);
			if(ret == -1){
				log_debug("raw_get error!");
			}else if(ret == 0){
				log_debug("skip not found");
			}else{
				log_debug("SYNC");
				link->send(log.repr(), val);
			}
			break;
		case BinlogCommand::KDEL:
			link->send(log.repr());
			break;
		default:
			log_debug("not found");
			break;
	}
	return 0;
}

void BackendSync::Client::out_of_sync() {
	this->status = Client::OUT_OF_SYNC;
	Binlog noop(this->last_seq, BinlogType::CTRL, BinlogCommand::NONE, "OUT_OF_SYNC");
	link->send(noop.repr());
}
