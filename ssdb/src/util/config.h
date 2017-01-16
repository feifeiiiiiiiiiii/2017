#ifndef UTIL__CONFIG_H
#define UTIL__CONFIG_H

#include <iostream>
#include <fstream>

class Config {
public:
    Config();
    std::string data_dir;
    std::string ip;
    int port;
    int cache_size;
    int block_size;
    int write_buffer_size;
    std::string compression;
	static Config* load(const char *filename);
};

#endif