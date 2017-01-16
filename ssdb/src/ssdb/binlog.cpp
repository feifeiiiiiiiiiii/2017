#include "binlog.h"
#include "const.h"
#include "../util/log.h"

static inline std::string encode_seq_key(uint64_t seq){
	seq = big_endian(seq);
	std::string ret;
	ret.push_back(DataType::SYNCLOG);
	ret.append((char *)&seq, sizeof(seq));
	return ret;
}

static inline uint64_t decode_seq_key(const leveldb::Slice &key){
	uint64_t seq = 0;
	if(key.size() == (sizeof(uint64_t) + 1) && key.data()[0] == DataType::SYNCLOG){
		seq = *((uint64_t *)(key.data() + 1));
		seq = big_endian(seq);
	}
	return seq;
}

Binlog::Binlog(uint64_t seq, char type, char cmd, const leveldb::Slice &key){
	buf.append((char *)(&seq), sizeof(uint64_t));
	buf.push_back(type);
	buf.push_back(cmd);
	buf.append(key.data(), key.size());
}

uint64_t Binlog::seq() const{
	return *((uint64_t *)(buf.data()));
}

char Binlog::type() const{
	return buf[sizeof(uint64_t)];
}

char Binlog::cmd() const{
	return buf[sizeof(uint64_t) + 1];
}

const Bytes Binlog::key() const{
	return Bytes(buf.data() + HEADER_LEN, buf.size() - HEADER_LEN);
}

int Binlog::load(const Bytes &s){
	if(s.size() < HEADER_LEN){
		return -1;
	}
	buf.assign(s.data(), s.size());
	return 0;
}

int Binlog::load(const leveldb::Slice &s){
	if(s.size() < HEADER_LEN){
		return -1;
	}
	buf.assign(s.data(), s.size());
	return 0;
}

int Binlog::load(const std::string &s){
	if(s.size() < HEADER_LEN){
		return -1;
	}
	buf.assign(s.data(), s.size());
	return 0;
}

BinlogQueue::BinlogQueue(leveldb::DB *db, int capacity) {
    this->db = db;
    this->capacity = capacity;
    this->last_seq = 0;
    this->tran_seq = 0;

    Binlog log;
    if(this->find_last(&log) == 1){
        this->last_seq = log.seq();
    }
}

BinlogQueue::~BinlogQueue() {
    db = NULL;
}

// leveldb put
void BinlogQueue::Put(const leveldb::Slice& key, const leveldb::Slice& value){
	batch.Put(key, value);
}

// leveldb delete
void BinlogQueue::Delete(const leveldb::Slice& key){
	batch.Delete(key);
}

void BinlogQueue::begin(){
    tran_seq = last_seq;
	batch.Clear();
}

leveldb::Status BinlogQueue::commit(){
	leveldb::WriteOptions write_opts;
	leveldb::Status s = db->Write(write_opts, &batch);
    if(s.ok()){
        last_seq = tran_seq;
        log_info("last_seq tran_seq %d", tran_seq);
        tran_seq = 0;
    }
	return s;
}

void BinlogQueue::rollback(){
    tran_seq = 0;
}

void BinlogQueue::add_log(char type, char cmd, const leveldb::Slice &key) {
    tran_seq++;
    Binlog log(tran_seq, type, cmd, key);
    batch.Put(encode_seq_key(tran_seq), log.repr());
}

void BinlogQueue::add_log(char type, char cmd, const std::string &key) {
    leveldb::Slice s(key);
    this->add_log(type, cmd, s);
}

int BinlogQueue::find_last(Binlog *log) const {
	uint64_t ret = 0;
	std::string key_str = encode_seq_key(UINT64_MAX);
	leveldb::ReadOptions iterate_options;
	leveldb::Iterator *it = db->NewIterator(iterate_options);
	it->Seek(key_str);
	if(!it->Valid()){
		// Iterator::prev requires Valid, so we seek to last
		it->SeekToLast();
	}else{
		// UINT64_MAX is not used 
		it->Prev();
	}
	if(it->Valid()){
		leveldb::Slice key = it->key();
		if(decode_seq_key(key) != 0){
			leveldb::Slice val = it->value();
			if(log->load(val) == -1){
				ret = -1;
			}else{
				ret = 1;
			}
		}
	}
	delete it;
	return ret;
}

