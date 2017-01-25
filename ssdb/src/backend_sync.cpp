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

	int r = uv_thread_create(&tid, BackendSync::_run_thread, (void *)arg);
    if(r != 0){
        log_error("can't create thread: %s", strerror(r));
        delete link;
    }
}

void BackendSync::_run_thread(void *arg) {
    log_debug("BackendSync thread run");
}
