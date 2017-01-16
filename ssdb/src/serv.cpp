#include "serv.h"
#include "net/server.h"
#include "net/proc.h"

DEF_PROC(get);
DEF_PROC(set);
DEF_PROC(del);

#define REG_PROC(c, f)     net->proc_map.set_proc(#c, f, proc_##c)

void SSDBServer::reg_procs(NetworkServer *net) {
    REG_PROC(get, "rt");
    REG_PROC(set, "wt");
    REG_PROC(del, "wt");
}

SSDBServer::SSDBServer(SSDB *ssdb, NetworkServer *net){
	this->ssdb = (SSDBImpl *)ssdb;
	net->data = this;
    this->reg_procs(net);
}

SSDBServer::~SSDBServer(){
}

