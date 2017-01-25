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
#include "net/link.h"

class BackendSync{
private:
	SSDBImpl *ssdb;
 	uv_thread_t tid;
	static void _run_thread(void *arg);
    struct run_arg{
        const Link *link;
        const BackendSync *backend;
    };

public:
	BackendSync(SSDBImpl *ssdb);
	~BackendSync();

    void proc(const Link *link);
};

#endif
