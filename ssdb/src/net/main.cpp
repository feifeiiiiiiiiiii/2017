#include "server.h"

int main() {
    NetworkServer* serv = NetworkServer::init();
    serv->serve();
}
