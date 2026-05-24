#include "server.h"
#include "resp_parser.h"
#include <iostream>

using namespace std;

constexpr int MAX_EVENTS=64;

inline int Server::evict_LRU() {
    int lru_node = LRU_FD[lru_tail].prev_index;
    if(time(nullptr) - LRU_FD[lru_node].last_accessed > 60){
        LRU_FD[LRU_FD[lru_node].prev_index].next_index = lru_tail;
        LRU_FD[lru_tail].prev_index = LRU_FD[lru_node].prev_index;
        LRU_FD[lru_node].next_index = -1;
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL,LRU_FD[lru_node].fd, nullptr);
        fd_list.erase(LRU_FD[lru_node].fd);
        close(LRU_FD[lru_node].fd);
        return lru_node;
    }
    else return -1;
}

inline int Server::get_available_node() {
    if (free_list != -1) {
        int node = free_list;
        free_list = LRU_FD[free_list].next_index;
        return node;
    }
    else return evict_LRU();
}

Server::Server(int p) {
    //Implementation of free list architecture in order to eliminate need of a separate vector to track free nodes
    for(int i=0 ; i<lru_head-1 ; i++){
        LRU_FD[i].next_index = i+1;
    }
    LRU_FD[lru_head-1].next_index = -1;
    port = p;
    server_fd = -1;

    //Setup of dummy head and tail nodes
    LRU_FD[lru_head].next_index = lru_tail;
    LRU_FD[lru_head].prev_index = -1;
    LRU_FD[lru_tail].prev_index = lru_head;
    LRU_FD[lru_tail].next_index = -1;
}

Server::~Server() {
    if (server_fd != -1) {
        close(server_fd);
        cout << "Server socket closed.\n";
    }
}

bool Server::start() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);    //Creating the server socket
    if(server_fd < 0) return false;

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));     //the pointer to opt and its size is passed to the feature since it doesn't accept boolean values and it needs to be given the arguments of how many bytes to read from where.
    // Prevents the "Address already in use" error if the server crashes and restarts quickly

    struct sockaddr_in address;     //Assigning address to the server_fd
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) return false;      //Binding the port to the server_fd
    if(listen(server_fd, 3) < 0) return false;

    cout << "Successfully connected to Port " << port << "\n";
    return true;
}

void Server::run() {
    epoll_fd = epoll_create1(0);    //Creating the epoll instance
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
                if(client_socket<0) continue;
                int new_node;
                if((new_node = get_available_node())>=0){
                    ev.events = EPOLLIN;
                    ev.data.fd = client_socket;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &ev);
                    // cout << "New client connected on socket " << client_socket << "\n";
                    LRU_FD[new_node].prev_index = lru_head;
                    LRU_FD[new_node].next_index = LRU_FD[lru_head].next_index;
                    LRU_FD[LRU_FD[lru_head].next_index].prev_index = new_node;
                    LRU_FD[lru_head].next_index = new_node;
                    LRU_FD[new_node].fd = client_socket;
                    LRU_FD[new_node].last_accessed = time(nullptr);
                    fd_list[client_socket] = new_node;
                }
                else{
                    string reject_msg = "-ERR max number of clients reached\r\n";
                    write(client_socket, reject_msg.c_str(), reject_msg.length());
                    close(client_socket);
                }
            }
            else{
                int client_fd = events[i].data.fd;
                char buff[1024] = {};
                int bytes_read = read(client_fd, buff, 1024);
                if (bytes_read <= 0) {
                    // cout << "Client disconnected.\n";
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
                    close(client_fd);
                    int node_index = fd_list[client_fd];
                    LRU_FD[LRU_FD[node_index].next_index].prev_index = LRU_FD[node_index].prev_index;
                    LRU_FD[LRU_FD[node_index].prev_index].next_index = LRU_FD[node_index].next_index;
                    LRU_FD[node_index].prev_index = -1;
                    LRU_FD[node_index].next_index = free_list;
                    free_list = node_index;
                    fd_list.erase(client_fd);
                }
                else{
                    int node_index;
                    node_index = fd_list[client_fd];
                    LRU_FD[LRU_FD[node_index].prev_index].next_index = LRU_FD[node_index].next_index;
                    LRU_FD[LRU_FD[node_index].next_index].prev_index = LRU_FD[node_index].prev_index;
                    LRU_FD[node_index].next_index = LRU_FD[lru_head].next_index;
                    LRU_FD[node_index].prev_index = lru_head;
                    LRU_FD[LRU_FD[lru_head].next_index].prev_index = node_index;
                    LRU_FD[lru_head].next_index = node_index;
                    LRU_FD[node_index].last_accessed = time(nullptr);
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
                    else if (cmd.name == "EXISTS" && !cmd.args.empty()){
                        string key = cmd.args[0];
                        if(db.exists(key)) response = ":1\r\n";
                        else response = ":0\r\n";
                    }

                    else if (cmd.name == "DEL" && !cmd.args.empty()){
                        string key = cmd.args[0];
                        if(db.del(key)) response = ":1\r\n";
                        else response = ":0\r\n";
                    }

                    else if (cmd.name == "RENAME" && cmd.args.size()>=2){
                        if(db.rename(cmd.args[0],cmd.args[1])) response = "+OK\r\n";
                        else response = "-ERR no such key\r\n";
                    }

                    else if (cmd.name == "INCR" && !cmd.args.empty()){
                        string result = db.incr(cmd.args[0]);
                        if(result == "ERR") response = "-ERR value is not an integer or out of range\r\n";
                        else response = ":" + result + "\r\n";
                    }

                    else if (cmd.name == "DBSIZE") response = ":" + to_string(db.size()) + "\r\n";

                    else if (!cmd.name.empty()) response = "-ERR unknown command '" + cmd.name + "'\r\n";
            
                    else response = "-ERR invalid request\r\n";

                    ssize_t bytes_written = write(client_fd, response.c_str(), response.length());
                    if (bytes_written < 0) cerr << "Error writing to socket " << client_fd << "\n";
                    else if (bytes_written < response.length()) cerr << "Warning: Partial write on socket " << client_fd << "\n";
                }
            }
        }
    }
}