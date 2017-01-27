#ifndef SKDB_SSDB_H_
#define SKDB_SSDB_H_

#include <vector>
#include <string>
#include "iterator.h"

class Bytes;
class Config;

class SSDB {
public:
    SSDB() {}
    virtual ~SSDB() {}

	static SSDB* open(const Config *cf, std::string dir);
    virtual Iterator* iterator(const std::string &start, const std::string &end, uint64_t limit) = 0;
    virtual int set(const Bytes &key, const Bytes &val) = 0;
    virtual int del(const Bytes &key) = 0;
    virtual int get(const Bytes &key, std::string *val) = 0;

    virtual int raw_set(const Bytes &key, const Bytes &val) = 0;
    virtual int raw_del(const Bytes &key) = 0;
    virtual int raw_get(const Bytes &key, std::string *val) = 0;

private:

};

#endif
