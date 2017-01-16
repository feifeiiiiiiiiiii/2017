#include "net/server.h"
#include "ssdb/ssdb.h"
#include "serv.h"
#include "util/config.h"

int run(int argc, char **argv) {
    SSDB *data_db = NULL;
    Config* cf = NULL;

    if(argc <= 1) {
        fprintf(stderr, "need more args");
        return 0;
    }
    std::string filename = argv[1];
    cf = Config::load(filename.c_str());

    data_db = SSDB::open(cf);
    if(!data_db){
        log_fatal("could not open data db");
        fprintf(stderr, "could not open data db");
        exit(1);
    }

    SSDBServer *server;
    NetworkServer *net = NULL;
    net = NetworkServer::init(cf);

    server = new SSDBServer(data_db, net);

    log_info("ssdb server started.");
    net->serve();

    delete net;
    delete cf;
    delete server;
    delete data_db;

    return 1;
}

int main(int argc, char **argv) {
    run(argc, argv);
}
