#include "serv.h"
#include "net/server.h"
#include "net/proc.h"
#include "slave.h"

DEF_PROC(get);
DEF_PROC(set);
DEF_PROC(del);
DEF_PROC(sync140);
DEF_PROC(hset);
DEF_PROC(hget);

#define REG_PROC(c, f)     net->proc_map.set_proc(#c, f, proc_##c)

void SSDBServer::reg_procs(NetworkServer *net) {
    REG_PROC(get, "rt");
    REG_PROC(set, "wt");
    REG_PROC(del, "wt");
    REG_PROC(hset, "wt");
    REG_PROC(hget, "wt");
    REG_PROC(sync140, "wt");
}

SSDBServer::SSDBServer(SSDB *ssdb, SSDB *meta, NetworkServer *net, const Config *cf){
	this->ssdb = (SSDBImpl *)ssdb;
    this->meta = (SSDBImpl *)meta; // only use by slave
	net->data = this;
    this->reg_procs(net);

    // backend sync 
    backend_sync = new BackendSync(this->ssdb);
    // replication
    Slave *slave = new Slave(ssdb, meta, cf->master_ip, cf->master_port);
	slave->start();
}

SSDBServer::~SSDBServer(){
}

int proc_sync140(NetworkServer *net, Link *link, const Request &req, Response *resp){
	SSDBServer *serv = (SSDBServer *)net->data;
    serv->backend_sync->proc(link, &req);
	return 0;
}
