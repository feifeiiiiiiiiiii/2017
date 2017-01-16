#ifndef SSKV_SSDB_IMPL_H_
#define SSKV_SSDB_IMPL_H_

#include <leveldb/db.h>
#include "ssdb.h"
#include "binlog.h"
#include "const.h"
#include "../util/log.h"
#include "../util/bytes.h"

inline
static leveldb::Slice slice(const Bytes &b){
    return leveldb::Slice(b.data(), b.size());
}

class SSDBImpl : public SSDB {
private:
    friend class SSDB;
    leveldb::DB* ldb;
    leveldb::Options options;

public:
    BinlogQueue *binlogs;

    SSDBImpl();
    virtual ~SSDBImpl();

    virtual int set(const Bytes &key, const Bytes &val);
    virtual int del(const Bytes &key);
    virtual int get(const Bytes &key, std::string *val);

    virtual int raw_set(const Bytes &key, const Bytes &val);
    virtual int raw_del(const Bytes &key);
    virtual int raw_get(const Bytes &key, std::string *val);
};

#endif
