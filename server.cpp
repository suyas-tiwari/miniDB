#include "server.h"
#include "resp_parser.h"
#include <iostream>

using namespace std;

constexpr int MAX_EVENTS=64;

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
    int epoll_fd = epoll_create1(0);
    if(epoll_fd<0){
        cout<<"There is some error in the program\n";
        return;
    }

    struct epoll_event ev,events[MAX_EVENTS];
    ev.data.fd=server_fd;
    ev.events=EPOLLIN;

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

    cout << "Waiting for connections on port " << port << "...\n";
    
    while(true) {
        // Waiting for a new client
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        for(int i=0;i<nfds;i++){
            if (events[i].data.fd == server_fd) {
                int client_socket = accept(server_fd, nullptr, nullptr);
                ev.events = EPOLLIN;
                ev.data.fd = client_socket;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &ev);
                cout << "New client connected on socket " << client_socket << "\n";
            }
            
            else{
                int client_fd = events[i].data.fd;
                char buff[1024] = {};
                int bytes_read = read(client_fd, buff, 1024);
                if (bytes_read <= 0) {
                    cout << "Client disconnected.\n";
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
                    close(client_fd);
                }
                else{
                    string input(buff, bytes_read);
                    Command cmd = parseRESP(input);
                string response = "";

                if (cmd.name == "PING") response = "+PONG\r\n";
                else if (cmd.name == "ECHO" && !cmd.args.empty()) {
                    string msg = cmd.args[0];
                    response = "$" + to_string(msg.length()) + "\r\n" + msg + "\r\n";
                }
                else if (cmd.name == "SET" && cmd.args.size() >= 2) {
                    db.set(cmd.args[0], cmd.args[1]);
                    response = "+OK\r\n";
                }
                else if (cmd.name == "GET" && !cmd.args.empty()) {
                    string key = cmd.args[0];
                    if (db.exists(key)) {
                        string val = db.get(key);
                        response = "$" + to_string(val.length()) + "\r\n" + val + "\r\n";
                    }
                    else response = "$-1\r\n";
                }

                else if (!cmd.name.empty()) response = "-ERR unknown command '" + cmd.name + "'\r\n";
            
                else response = "-ERR invalid request\r\n";

                write(client_fd, response.c_str(), response.length());
                }
            }
        }
    }
}