#include "ssdb_impl.h"
#include "../util/config.h"

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

SSDB* SSDB::open(const Config *cf) {
	SSDBImpl *ssdb = new SSDBImpl();
	ssdb->options.create_if_missing = true;
	ssdb->options.block_size = cf->block_size * 1024 * 1024;
	ssdb->options.write_buffer_size = cf->write_buffer_size * 1024 * 1024;
    if(cf->compression == "yes") {
        ssdb->options.compression = leveldb::kSnappyCompression;
    }

	leveldb::Status status;

	status = leveldb::DB::Open(ssdb->options, cf->data_dir, &ssdb->ldb);
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
