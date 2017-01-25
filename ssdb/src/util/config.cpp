/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "json.hpp"
#include "config.h"

using json = nlohmann::json;

Config::Config() {
    ip = "127.0.0.1";
    port = 8888;
    cache_size = 500;
    block_size = 32;
    compression = "yes";
    write_buffer_size = 64;
    data_dir = "ssdb/data";
    meta_dir = "ssdb/meta";
    master_port = -1;
    master_ip = "";
}

Config* Config::load(const char *filename){
	Config *root = new Config();

    std::ifstream fp(filename);
    json js;
    fp >> js;

    if(js["data_dir"].is_string()) {
        root->data_dir = js["data_dir"];
    }

    if(js["meta_dir"].is_string()) {
        root->meta_dir = js["meta_dir"];
    }

    if(js["server"].is_object()) {
        if(js["server"]["ip"].is_string()) {
            root->ip = js["server"]["ip"];
        }
        if(js["server"]["port"].is_number()) {
            root->port = js["server"]["port"];
        }
    }

    if(js["leveldb"].is_object()) {
        if(js["leveldb"]["cache_size"].is_number()) {
            root->cache_size = js["leveldb"]["cache_size"];
        }

        if(js["leveldb"]["block_size"].is_number()) {
            root->block_size = js["leveldb"]["block_size"];
        }

        if(js["leveldb"]["write_buffer_size"].is_number()) {
            root->write_buffer_size = js["leveldb"]["write_buffer_size"];
        }

        if(js["leveldb"]["compression"].is_string()) {
            root->compression = js["leveldb"]["compression"];
        }
    }

    if(js["replication"].is_object()) {
        if(js["replication"]["slaveof"].is_object()) {
            if(js["replication"]["slaveof"]["port"].is_number()) {
                root->master_port = js["replication"]["slaveof"]["port"];
            }
            if(js["replication"]["slaveof"]["ip"].is_string()) {
                root->master_ip = js["replication"]["slaveof"]["ip"];
            }
        }
    }
    return root;
}
