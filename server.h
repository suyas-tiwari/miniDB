#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

class Server {
private:
    int port;
    int server_fd;

public:
    Server(int p); 
    ~Server();     
    bool start();  
    void run();    
};