#include "net/server.h"
#include "ssdb/ssdb.h"
#include "serv.h"
#include "util/config.h"
#include "slave.h"

int run(int argc, char **argv) {
    SSDB *data_db = NULL;
    SSDB *meta_db = NULL;
    Config* cf = NULL;

    if(argc <= 1) {
        fprintf(stderr, "need more args");
        return 0;
    }
    std::string filename = argv[1];
    cf = Config::load(filename.c_str());

    data_db = SSDB::open(cf, cf->data_dir);
    if(!data_db){
        log_fatal("could not open data db");
        fprintf(stderr, "couldnot open data db");
        exit(1);
    }

    meta_db = SSDB::open(cf, cf->meta_dir);
    if(!data_db){
        log_fatal("could not open meta db");
        fprintf(stderr, "couldnot open meta db");
        exit(1);
    }

    SSDBServer *server;
    NetworkServer *net = NULL;
    net = NetworkServer::init(cf);

    server = new SSDBServer(data_db, meta_db, net, cf);

    log_info("ssdb server started.");
    net->serve();

    delete net;
    delete cf;
    delete server;
    delete data_db;
    delete meta_db;

    return 1;
}

int main(int argc, char **argv) {
    run(argc, argv);
}
