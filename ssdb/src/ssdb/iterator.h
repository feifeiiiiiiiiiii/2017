/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#ifndef SSDB_ITERATOR_H_
#define SSDB_ITERATOR_H_

#include <inttypes.h>
#include <string>
#include "../util/bytes.h"
#include "const.h"

namespace leveldb{
	class Iterator;
}

class Iterator{
public:
	enum Direction{
		FORWARD, BACKWARD
	};
	Iterator(leveldb::Iterator *it,
			const std::string &end,
			uint64_t limit,
			Direction direction=Iterator::FORWARD);
	~Iterator();
	bool skip(uint64_t offset);
	bool next();
	Bytes key();
	Bytes val();
private:
	leveldb::Iterator *it;
	std::string end;
	uint64_t limit;
	bool is_first;
	int direction;
};


class KIterator{
public:
	std::string key;
	std::string val;

	KIterator(Iterator *it);
	~KIterator();
	void return_val(bool onoff);
	bool next();
private:
	Iterator *it;
	bool return_val_;
};

#endif
