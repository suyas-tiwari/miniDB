#include "server.h"
#include <iostream>

using namespace std;

Server::Server(int p) {
    port = p;
    server_fd = -1;
}

Server::~Server() {
    if (server_fd != -1) {
        close(server_fd);
        cout << "Server socket closed.\n";
    }
}

bool Server::start() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0) return false;

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    // Prevents the "Address already in use" error if the server crashes and restarts quickly

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) return false;
    if(listen(server_fd, 3) < 0) return false;

    cout << "Successfully connected to Port " << port << "\n";
    return true;
}

void Server::run() {
    cout << "Waiting for connections on port " << port << "...\n";
    
    while(true) {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0) continue;
        
        cout << "New client connected!\n";

        char buff[1024] = {};
        int bytes_read = read(client_socket, buff, 1024);
        
        if (bytes_read > 0) {
            cout << "Received " << bytes_read << " bytes.\n";
            string input(buff, bytes_read);
            
            cout << "Raw packet:\n" << input << "\n";

            string response = "+OK\r\n";
            write(client_socket, response.c_str(), response.length());
        }

        close(client_socket); 
    }
}