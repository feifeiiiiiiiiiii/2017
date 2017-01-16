#ifndef SERV_H_
#define SERV_H_

#include <map>
#include <vector>
#include <string>
#include "ssdb/ssdb_impl.h"
#include "net/server.h"

class SSDBServer {
private:
public:
    SSDBImpl *ssdb;
    SSDBServer(SSDB *ssdb, NetworkServer *net);
    void reg_procs(NetworkServer *net);
    ~SSDBServer();
};

#define CHECK_NUM_PARAMS(n) do{ \
    if(req.size() < n){ \
	    resp->push_back("client_error"); \
	    resp->push_back("wrong number of arguments"); \
	    return 0; \
		} \
	}while(0)

#endif
