#include "ssdb_impl.h"
#include "../util/config.h"

static int hset_one(SSDBImpl *ssdb, const Bytes &name, const Bytes &key, const Bytes &val, char log_type);
static int hdel_one(SSDBImpl *ssdb, const Bytes &name, const Bytes &key, char log_type);
static int incr_hsize(SSDBImpl *ssdb, const Bytes &name, int64_t incr);

inline static
std::string encode_hsize_key(const Bytes &name){
	std::string buf;
	buf.append(1, DataType::HSIZE);
	buf.append(name.data(), name.size());
	return buf;
}

inline static
int decode_hsize_key(const Bytes &slice, std::string *name){
	Decoder decoder(slice.data(), slice.size());
	if(decoder.skip(1) == -1){
		return -1;
	}
	if(decoder.read_data(name) == -1){
		return -1;
	}
	return 0;
}

inline static
std::string encode_hash_key(const Bytes &name, const Bytes &key){
	std::string buf;
	buf.append(1, DataType::HASH);
	buf.append(1, (uint8_t)name.size());
	buf.append(name.data(), name.size());
	buf.append(1, '=');
	buf.append(key.data(), key.size());
	return buf;
}

inline static
int decode_hash_key(const Bytes &slice, std::string *name, std::string *key){
	Decoder decoder(slice.data(), slice.size());
	if(decoder.skip(1) == -1){
		return -1;
	}
	if(decoder.read_8_data(name) == -1){
		return -1;
	}
	if(decoder.skip(1) == -1){
		return -1;
	}
	if(decoder.read_data(key) == -1){
		return -1;
	}
	return 0;
}

static int incr_hsize(SSDBImpl *ssdb, const Bytes &name, int64_t incr){
	int64_t size = ssdb->hsize(name);
	size += incr;
	std::string size_key = encode_hsize_key(name);
	if(size == 0){
		ssdb->binlogs->Delete(size_key);
	}else{
		ssdb->binlogs->Put(size_key, leveldb::Slice((char *)&size, sizeof(int64_t)));
	}
	return 0;
}

static inline
std::string encode_kv_key(const Bytes &key){
	std::string buf;
	buf.append(1, DataType::KV);
	buf.append(key.data(), key.size());
	return buf;
}

static inline
int decode_kv_key(const Bytes &slice, std::string *key){
	Decoder decoder(slice.data(), slice.size());
	if(decoder.skip(1) == -1){
		return -1;
	}
	if(decoder.read_data(key) == -1){
		return -1;
	}
	return 0;
}

SSDBImpl::SSDBImpl() {
    ldb = NULL;
    binlogs = NULL;
}

SSDBImpl::~SSDBImpl() {
    if(ldb) {
        delete ldb;
    }
}

SSDB* SSDB::open(const Config *cf, std::string dir) {
	SSDBImpl *ssdb = new SSDBImpl();
	ssdb->options.create_if_missing = true;
	ssdb->options.block_size = cf->block_size * 1024 * 1024;
	ssdb->options.write_buffer_size = cf->write_buffer_size * 1024 * 1024;
    if(cf->compression == "yes") {
        ssdb->options.compression = leveldb::kSnappyCompression;
    }

	leveldb::Status status;

	status = leveldb::DB::Open(ssdb->options, dir, &ssdb->ldb);
	if(!status.ok()){
		log_error("open db failed: %s", status.ToString().c_str());
		goto err;
	}

    ssdb->binlogs = new BinlogQueue(ssdb->ldb);

	return ssdb;
err:
	if(ssdb){
		delete ssdb;
	}
	return NULL;
}

int SSDBImpl::set(const Bytes &key, const Bytes &val) {
    if(key.empty()) {
        log_error("empty key");
        return 0;
    }

    Transaction trans(binlogs);

    std::string buf = encode_kv_key(key);
    binlogs->Put(buf, slice(val));
    binlogs->add_log(BinlogType::SYNC, BinlogCommand::KSET, buf);
    leveldb::Status s = binlogs->commit();
    if(!s.ok()){
        return -1;
    }
    return 1;
}

int SSDBImpl::del(const Bytes &key) {
    Transaction trans(binlogs);

    std::string buf = encode_kv_key(key);
    binlogs->Delete(buf);
    binlogs->add_log(BinlogType::SYNC, BinlogCommand::KDEL, buf);
    leveldb::Status s = binlogs->commit();
    if(!s.ok()){
        log_error("del error: %s", s.ToString().c_str());
        return -1;
    }
    return 1;
}

int SSDBImpl::get(const Bytes &key, std::string *val) {
	std::string buf = encode_kv_key(key);

	leveldb::Status s = ldb->Get(leveldb::ReadOptions(), buf, val);
	if(s.IsNotFound()){
		return 0;
	}
	if(!s.ok()){
		return -1;
	}
	return 1;
}

int SSDBImpl::hset(const Bytes &name, const Bytes &key, const Bytes &val, char log_type){
	Transaction trans(binlogs);

	int ret = hset_one(this, name, key, val, log_type);
	if(ret >= 0){
		if(ret > 0){
			if(incr_hsize(this, name, ret) == -1){
				return -1;
			}
		}
		leveldb::Status s = binlogs->commit();
		if(!s.ok()){
			return -1;
		}
	}
	return ret;
}

int SSDBImpl::hget(const Bytes &name, const Bytes &key, std::string *val){
	std::string dbkey = encode_hash_key(name, key);
	leveldb::Status s = ldb->Get(leveldb::ReadOptions(), dbkey, val);
	if(s.IsNotFound()){
		return 0;
	}
	if(!s.ok()){
		log_error("%s", s.ToString().c_str());
		return -1;
	}
	return 1;
}

int64_t SSDBImpl::hsize(const Bytes &name){
	std::string size_key = encode_hsize_key(name);
	std::string val;
	leveldb::Status s;

	s = ldb->Get(leveldb::ReadOptions(), size_key, &val);
	if(s.IsNotFound()){
		return 0;
	}else if(!s.ok()){
		return -1;
	}else{
		if(val.size() != sizeof(uint64_t)){
			return 0;
		}
		int64_t ret = *(int64_t *)val.data();
		return ret < 0? 0 : ret;
	}
}

/* raw operates */
int SSDBImpl::raw_set(const Bytes &key, const Bytes &val){
	leveldb::WriteOptions write_opts;
	leveldb::Status s = ldb->Put(write_opts, slice(key), slice(val));
	if(!s.ok()){
		log_error("set error: %s", s.ToString().c_str());
		return -1;
	}
	return 1;
}

int SSDBImpl::raw_del(const Bytes &key){
	leveldb::WriteOptions write_opts;
	leveldb::Status s = ldb->Delete(write_opts, slice(key));
	if(!s.ok()){
		log_error("del error: %s", s.ToString().c_str());
		return -1;
	}
	return 1;
}

int SSDBImpl::raw_get(const Bytes &key, std::string *val){
	leveldb::ReadOptions opts;
	opts.fill_cache = false;
	leveldb::Status s = ldb->Get(opts, slice(key), val);
	if(s.IsNotFound()){
		return 0;
	}
	if(!s.ok()){
		log_error("get error: %s", s.ToString().c_str());
		return -1;
	}
	return 1;
}

Iterator* SSDBImpl::iterator(const std::string &start, const std::string &end, uint64_t limit){
    leveldb::Iterator *it;
    leveldb::ReadOptions iterate_options;
    iterate_options.fill_cache = false;
    it = ldb->NewIterator(iterate_options);
    it->Seek(start);
    if(it->Valid() && it->key() == start){
        it->Next();
    }
    return new Iterator(it, end, limit);
}

// returns the number of newly added items
static int hset_one(SSDBImpl *ssdb, const Bytes &name, const Bytes &key, const Bytes &val, char log_type){
	if(name.empty() || key.empty()){
		log_error("empty name or key!");
		return -1;
	}
	if(name.size() > SSDB_KEY_LEN_MAX ){
		log_error("name too long! %s", hexmem(name.data(), name.size()).c_str());
		return -1;
	}
	if(key.size() > SSDB_KEY_LEN_MAX){
		log_error("key too long! %s", hexmem(key.data(), key.size()).c_str());
		return -1;
	}
	int ret = 0;
	std::string dbval;
	if(ssdb->hget(name, key, &dbval) == 0){ // not found
		std::string hkey = encode_hash_key(name, key);
		ssdb->binlogs->Put(hkey, slice(val));
		ssdb->binlogs->add_log(log_type, BinlogCommand::HSET, hkey);
		ret = 1;
	}else{
		if(dbval != val){
			std::string hkey = encode_hash_key(name, key);
			ssdb->binlogs->Put(hkey, slice(val));
			ssdb->binlogs->add_log(log_type, BinlogCommand::HSET, hkey);
		}
		ret = 0;
	}
	return ret;
}

static int hdel_one(SSDBImpl *ssdb, const Bytes &name, const Bytes &key, char log_type){
	if(name.size() > SSDB_KEY_LEN_MAX ){
		log_error("name too long! %s", hexmem(name.data(), name.size()).c_str());
		return -1;
	}
	if(key.size() > SSDB_KEY_LEN_MAX){
		log_error("key too long! %s", hexmem(key.data(), key.size()).c_str());
		return -1;
	}
	std::string dbval;
	if(ssdb->hget(name, key, &dbval) == 0){
		return 0;
	}

	std::string hkey = encode_hash_key(name, key);
	ssdb->binlogs->Delete(hkey);
	ssdb->binlogs->add_log(log_type, BinlogCommand::HDEL, hkey);
	
	return 1;
}