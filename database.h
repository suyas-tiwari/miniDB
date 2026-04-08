#pragma once
#include <string>
#include <unordered_map>
#include <fstream>
#include "resp_parser.h"

class Database {
    private:
        std::unordered_map<std::string, std::string> store;
        std::ofstream aof_writer;
        std::ifstream aof_reader;
    public:
        Database();
        ~Database();
        void set(const std::string& key, const std::string& value);
        std::string get(const std::string& key);
        bool exists(const std::string& key);
};