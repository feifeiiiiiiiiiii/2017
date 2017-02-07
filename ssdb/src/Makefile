OBJS = serv.o link_redis.o resp.o server.o \
	   binlog.o ssdb_impl.o bytes.o log.o config.o \
	   link.o proc.o proc_kv.o ssdb_server.o slave.o \
	   backend_sync.o iterator.o

LIBS = -luv -lleveldb -std=c++11 -g
LEVELDB_INC = ${LEVELDB_PATH}/include
LIBUV_INC = ${LIBUV_PATH}/include
LIB_DIRS = -L${LEVELDB_PATH}/out-static -L${LIBUV_PATH}/.libs

EXES = ../ssdb-server
CXX = g++

all: ${OBJS}
	${CXX} -o ../ssdb-server ${OBJS} ${LIBS} -I${LIBUV_INC} -I${LEVELDB_INC} ${LIB_DIRS}

ssdb_server.o: ssdb_server.cpp
	${CXX} -c ssdb_server.cpp -I${LIBUV_INC} -I${LEVELDB_INC}

proc_kv.o: proc_kv.cpp
	${CXX} -c proc_kv.cpp -I${LEVELDB_INC} -I${LIBUV_INC}

serv.o: serv.h serv.cpp ssdb/ssdb_impl.h net/server.h net/proc.h proc_kv.cpp backend_sync.h
	${CXX} -c serv.cpp -I${LEVELDB_INC} -I${LIBUV_INC}

server.o: net/link.h net/proc.h util/config.h
	${CXX} -c net/server.cpp -I${LIBUV_INC} -I${LEVELDB_INC}

link_redis.o: util/bytes.h
	${CXX} -c net/link_redis.cpp

resp.o: net/resp.h
	${CXX} -c net/resp.cpp

binlog.o: ssdb/binlog.h util/bytes.h util/thread.h
	${CXX} -c ssdb/binlog.cpp -I${LEVELDB_INC}

ssdb_impl.o: ssdb/ssdb.h ssdb/ssdb_impl.h util/log.h util/bytes.h ssdb/iterator.h
	${CXX} -c ssdb/ssdb_impl.cpp -I${LEVELDB_INC}

bytes.o: util/bytes.h util/strings.h
	${CXX} -c util/bytes.cpp

log.o: util/log.h
	${CXX} -c util/log.cpp

config.o: util/config.h util/json.hpp
	${CXX} -c util/config.cpp -std=c++11

link.o: net/proc.h util/bytes.h net/link_redis.h serv.h include.h
	${CXX} -c net/link.cpp -I${LEVELDB_INC} -I${LIBUV_INC}

proc.o: net/proc.h net/resp.h util/bytes.h
	${CXX} -c net/proc.cpp -I${LIBUV_INC}

slave.o: slave.h ssdb/ssdb_impl.h ssdb/binlog.h slave.cpp
	${CXX} -c slave.cpp -I${LEVELDB_INC} -I${LIBUV_INC}

backend_sync.o: include.h ssdb/ssdb_impl.h net/link.h ssdb/binlog.h backend_sync.cpp
	${CXX} -c backend_sync.cpp -I${LEVELDB_INC} -I${LIBUV_INC}

iterator.o: util/bytes.h ssdb/iterator.h ssdb/const.h
	${CXX} -c ssdb/iterator.cpp -I${LEVELDB_INC}

clean:
	rm -f ${OBJS}

