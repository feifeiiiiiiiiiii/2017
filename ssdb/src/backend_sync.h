/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#ifndef SSDB_BACKEND_SYNC_H_
#define SSDB_BACKEND_SYNC_H_

#include "include.h"
#include <vector>
#include <string>
#include <map>
#include <uv.h>

#include "ssdb/ssdb_impl.h"
#include "ssdb/binlog.h"
#include "ssdb/iterator.h"
#include "net/link.h"
#include "net/proc.h"

class BackendSync{
private:
    struct Client;
	SSDBImpl *ssdb;
	static void _run_thread(void *arg);
    struct run_arg{
        const Link *link;
        const BackendSync *backend;
		std::vector<Bytes> req;
    };
	Mutex mutex;
	std::map<pthread_t, Client *> workers;

public:
	BackendSync(SSDBImpl *ssdb);
	~BackendSync();

    void proc(const Link *link, const std::vector<Bytes> *req);
};

struct BackendSync::Client{
	static const int INIT = 0;
	static const int OUT_OF_SYNC = 1;
	static const int COPY = 2;
	static const int SYNC = 4;

	int status;
	Link *link;
	uint64_t last_seq;
	uint64_t last_noop_seq;
	std::string last_key;
	const BackendSync *backend;
	bool is_mirror;
    Iterator *iter;
	
	Client(const BackendSync *backend);
	~Client();
	void init(const std::vector<Bytes> *req);
	void reset();
	void noop();
	int copy();
	int sync(BinlogQueue *logs);
	void out_of_sync();
};

#endif
