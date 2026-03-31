#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <unordered_map>

class Server {
private:
    int port;
    int server_fd;
    std::unordered_map<std::string, std::string> data;

public:
    Server(int p); 
    ~Server();     
    bool start();  
    void run();    
};