/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "iterator.h"
#include "../util/log.h"
#include "../util/config.h"
#include "leveldb/iterator.h"

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

Iterator::Iterator(leveldb::Iterator *it,
		const std::string &end,
		uint64_t limit,
		Direction direction)
{
	this->it = it;
	this->end = end;
	this->limit = limit;
	this->is_first = true;
	this->direction = direction;
}

Iterator::~Iterator(){
	delete it;
}

Bytes Iterator::key(){
	leveldb::Slice s = it->key();
	return Bytes(s.data(), s.size());
}

Bytes Iterator::val(){
	leveldb::Slice s = it->value();
	return Bytes(s.data(), s.size());
}

bool Iterator::skip(uint64_t offset){
	while(offset-- > 0){
		if(this->next() == false){
			return false;
		}
	}
	return true;
}

bool Iterator::next(){
	if(limit == 0){
		return false;
	}
	if(is_first){
		is_first = false;
	}else{
		if(direction == FORWARD){
			it->Next();
		}else{
			it->Prev();
		}
	}

	if(!it->Valid()){
		// make next() safe to be called after previous return false.
		limit = 0;
		return false;
	}
	if(direction == FORWARD){
		if(!end.empty() && it->key().compare(end) > 0){
			limit = 0;
			return false;
		}
	}else{
		if(!end.empty() && it->key().compare(end) < 0){
			limit = 0;
			return false;
		}
	}
	limit --;
	return true;
}


/* KV */

KIterator::KIterator(Iterator *it){
	this->it = it;
	this->return_val_ = true;
}

KIterator::~KIterator(){
	delete it;
}

void KIterator::return_val(bool onoff){
	this->return_val_ = onoff;
}

bool KIterator::next(){
	while(it->next()){
		Bytes ks = it->key();
		Bytes vs = it->val();
		//dump(ks.data(), ks.size(), "z.next");
		//dump(vs.data(), vs.size(), "z.next");
		if(ks.data()[0] != DataType::KV){
			return false;
		}
		if(decode_kv_key(ks, &this->key) == -1){
			continue;
		}
		if(return_val_){
			this->val.assign(vs.data(), vs.size());
		}
		return true;
	}
	return  false;
}
