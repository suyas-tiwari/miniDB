#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <unordered_map>
#include <sys/epoll.h>
#include "database.h"

struct Node{
        int next_index;
        int prev_index;
        int fd;
        time_t last_accessed;
    };

class Server {
private:
    int port;
    int server_fd;
    Database db;
    Node LRU_FD[1000];
    unordered_map <int,int> fd_list;
    int free_list = 0;
    int lru_head = 998;     //using the node 998 as dummy head
    int lru_tail = 999;     //using the node 999 as dummy tail
    int get_available_node();
    int evict_LRU();
    int epoll_fd;

public:
    Server(int p); 
    ~Server();     
    bool start();  
    void run();    
};