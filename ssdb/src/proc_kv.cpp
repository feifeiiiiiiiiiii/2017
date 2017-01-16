/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
/* kv */
#include "serv.h"
#include "net/proc.h"
#include "net/server.h"
#include "net/resp.h"

int proc_get(NetworkServer *net, Link *link, const Request &req, Response *resp){
	SSDBServer *serv = (SSDBServer *)net->data;
    CHECK_NUM_PARAMS(2);

    std::string val;
    int ret = serv->ssdb->get(req[1], &val);
    resp->reply_get(ret, &val);
	return 0;
}

int proc_set(NetworkServer *net, Link *link, const Request &req, Response *resp){
	SSDBServer *serv = (SSDBServer *)net->data;
    CHECK_NUM_PARAMS(3);

    int ret = serv->ssdb->set(req[1], req[2]);
    if(ret == -1){
        resp->push_back("error");
    }else{
        resp->push_back("ok");
        resp->push_back("1");
    }
	return 0;
}

int proc_del(NetworkServer *net, Link *link, const Request &req, Response *resp){
    SSDBServer *serv = (SSDBServer *)net->data;
    CHECK_NUM_PARAMS(2);

    int ret = serv->ssdb->del(req[1]);
    if(ret == -1){
        resp->push_back("error");
    }else{
        resp->push_back("ok");
        resp->push_back("1");
    }
	return 0;
}
