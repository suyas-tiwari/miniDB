#pragma once
#include <string>
#include <unordered_map>
#include <fstream>
#include <mutex>
#include <chrono>
#include <thread>
#include "resp_parser.h"

class Database {
    private:
        std::unordered_map<std::string, std::string> store;
        std::ofstream aof_writer;
        std::ifstream aof_reader;
        std::mutex buffer_lock;
        std::string write_buffer;
        std::thread writer_thread;
        bool running;
        void write_to_disk();
        
    public:
        Database();
        ~Database();
        void set(const std::string& key, const std::string& value);
        bool del(const std::string& key);
        bool rename(const std::string& key, const std:: string& newKey);
        std::string incr(const std::string& key);
        std::string get(const std::string& key);
        bool exists(const std::string& key);
        int size();
};