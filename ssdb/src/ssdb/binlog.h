#ifndef SSDB_BINLOG_H_
#define SSDB_BINLOG_H_

#include <string>
#include <leveldb/db.h>
#include <leveldb/options.h>
#include <leveldb/slice.h>
#include <leveldb/status.h>
#include <leveldb/write_batch.h>
#include "../util/bytes.h"
#include "../util/thread.h"

class Binlog {
private:
    std::string buf;
    static const unsigned int HEADER_LEN = sizeof(uint64_t) + 2;
public:
    Binlog() {}
    Binlog(uint64_t seq, char type, char cmd, const leveldb::Slice &key);

    int load(const Bytes &s);
    int load(const leveldb::Slice &s);
    int load(const std::string &s);

    uint64_t seq() const;
    char type() const;
    char cmd() const;
    const Bytes key() const;

    const char* data() const{
        return buf.data();
    }
    int size() const{
        return (int)buf.size();
    }
    const std::string repr() const{
        return this->buf;
    }
};

class BinlogQueue {
private:
	leveldb::DB *db;
	int capacity;
    uint64_t last_seq;
    uint64_t tran_seq;
	leveldb::WriteBatch batch;
public:
	Mutex mutex;
	BinlogQueue(leveldb::DB *db, int capacity=20000000);
	~BinlogQueue();
	void begin();
	void rollback();
	leveldb::Status commit();

	// leveldb put
	void Put(const leveldb::Slice& key, const leveldb::Slice& value);
	// leveldb delete
	void Delete(const leveldb::Slice& key);

    void add_log(char type, char cmd, const leveldb::Slice &key);
    void add_log(char type, char cmd, const std::string &key);

    int find_last(Binlog *log) const;
    int get(uint64_t seq, Binlog *log) const;
    int find_next(uint64_t next_seq, Binlog *log) const;

};

class Transaction{
private:
	BinlogQueue *logs;
public:
	Transaction(BinlogQueue *logs){
		this->logs = logs;
		logs->mutex.lock();
		logs->begin();
	}
	
	~Transaction(){
		// it is safe to call rollback after commit
		logs->rollback();
		logs->mutex.unlock();
	}
};


#endif
