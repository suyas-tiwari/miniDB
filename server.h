#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <unordered_map>
#include <sys/epoll.h>
#include "database.h"

class Server {
private:
    int port;
    int server_fd;
    Database db;

public:
    Server(int p); 
    ~Server();     
    bool start();  
    void run();    
};