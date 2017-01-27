export BASE_DIR=${PWD}
export LEVELDB_PATH="${BASE_DIR}/deps/leveldb@master"
export LIBUV_PATH="${BASE_DIR}/deps/libuv@master"

all:
	cd "${LEVELDB_PATH}"; make;
	cd "${LIBUV_PATH}"; ./autogen.sh; ./configure; make;
	cd "src"; make;

clean:
	cd src; make clean;
	rm ssdb-server;
