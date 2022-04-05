/************************************************************************
> File Name:     Server.hpp
> Author:        Coyote
> Created Time:  Thu 25 Nov 2021 09:23:54 PM CST
> Description:   
 ************************************************************************/

#ifndef SERVER_H_
#define SERVER_H_

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <fstream>
#include "nlohmann/json.hpp"
#include "http/httpconn.h"
#include "threadPool/threadPool.hpp"

class Server {
public:
    Server(const char *);
    int acceptClient(struct sockaddr_in &, socklen_t &);
    
    void handler(int connfd);
    ~Server();
    Server(const Server &) =delete;
    Server &
        operator=(const Server &) =delete;

private:
    static const int MAX_FDS = 65535;
private:
    Http_Conn *m_hc;
    ThreadPool<Http_Conn> *m_pool;
    struct sockaddr_in server_addr;
    int listenfd;
};

Server::Server(const char *config)
{
    std::ifstream fin(config);
    nlohmann::json config_json;
    fin >> config_json;
    fin.close();

    server_addr.sin_family = AF_INET;

    std::string ip = config_json.at("ip");
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

    const short port = config_json.at("port");
    server_addr.sin_port = htons(port);

    bzero(&server_addr.sin_zero, 8);
    
    if ((listenfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        throw std::exception();
    }
    
    if (bind(listenfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        throw std::exception();
    }
    
    if (listen(listenfd, 10) == -1) {
        throw std::exception();
    }
    
    m_hc = new Http_Conn[MAX_FDS];
    m_pool = new ThreadPool<Http_Conn>();
}

int Server::acceptClient(struct sockaddr_in & client_addr, socklen_t & client_size) {
    int connfd = -1;
    
    connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_size);

    if (connfd == -1) {
        std::cout << "accept error\n";
        return -1;
    }
    
    std::cout << "connect with: " << inet_ntoa(client_addr.sin_addr);
    std::cout << ": " << ntohs(client_addr.sin_port) << std::endl;

    m_hc[0].init(connfd);
    return connfd;
}

Server::~Server()
{
    delete[] m_hc;
    delete m_pool;
    close(listenfd);
}

void Server::handler(int connfd) {
    m_pool->addTask(&Http_Conn::handler, &m_hc[0]);
}
#endif
