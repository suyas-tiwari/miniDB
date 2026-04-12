#pragma once
#include <string>
#include <unordered_map>
#include <fstream>
#include <mutex>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <sys/wait.h>
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
        static const int COMPACTION_THRESHOLD = 1 * 1024;
        void check_and_compact();
        int get_aof_size();
        int size_after_last_compaction = 0;

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