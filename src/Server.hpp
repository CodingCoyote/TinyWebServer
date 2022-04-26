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
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>
#include "nlohmann/json.hpp"
#include "http/httpconn.h"
#include "threadPool/threadPool.hpp"

class Server {
public:
    Server(const char *);
    int acceptClient();
    
    void handler();
    ~Server();
    Server(const Server &) =delete;
    Server &
        operator=(const Server &) =delete;
public:
    int setNonBlocking(int fd);
    void addfdToEpoll(int fd, bool oneshot, bool enable_et);
    void eventLoop();
private:
    static const int MAX_FDS = 65536;
    static const int MAX_EVENT_NUMBER = 10000;
private:
    Http_Conn *m_hc;
    ThreadPool<Http_Conn> *m_pool;
    int m_epollfd;
    epoll_event m_events[MAX_EVENT_NUMBER];
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
    if ((m_epollfd = epoll_create(5)) == -1) {
        throw std::exception();
    }
    addfdToEpoll(listenfd, false, false);
    Http_Conn::m_epollfd = m_epollfd;
}

int Server::acceptClient() {
    int connfd = -1;
    
    struct sockaddr_in client_addr;
    socklen_t client_size = sizeof(client_addr);
    connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_size);

    if (connfd == -1) {
        std::cout << "accept error\n";
        return -1;
    }
    
    std::cout << "connect with: " << inet_ntoa(client_addr.sin_addr);
    std::cout << ": " << ntohs(client_addr.sin_port) << std::endl;

    m_hc[connfd].init(connfd);
    return connfd;
}

Server::~Server()
{
    delete[] m_hc;
    delete m_pool;
    close(listenfd);
}

void Server::handler() {
    eventLoop();
}

int Server::setNonBlocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void Server::addfdToEpoll(int fd, bool oneshot, bool enable_et) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLRDHUP;
    if (enable_et) {
        event.events |= EPOLLET | EPOLLONESHOT;
    }
    if (oneshot) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(m_epollfd, EPOLL_CTL_ADD, fd, &event);
    setNonBlocking(fd);
}

void Server::eventLoop() {
    while (1) {
        int ret = epoll_wait(m_epollfd, m_events, MAX_EVENT_NUMBER, -1);
        if (ret <= 0) {
            std::cout << "epoll failure" << std::endl;
            break;
        }
        
        for (int i = 0; i < ret; ++i) {
            int sockfd = m_events[i].data.fd;
            if (sockfd == listenfd) {
                int connfd = acceptClient();
                addfdToEpoll(connfd, true, false);
            } else if (m_events[i].events & EPOLLRDHUP) {
                epoll_ctl(m_epollfd, EPOLL_CTL_DEL, sockfd, 0);
                close(sockfd);
            } else if (m_events[i].events & EPOLLIN) {
                m_pool->addTask(&Http_Conn::read_handler, &m_hc[sockfd]);
            } else if (m_events[i].events & EPOLLOUT) {
                m_pool->addTask(&Http_Conn::write_handler, &m_hc[sockfd]);
            } else {
                std::cout << "event can't handle\n";
            }
        }
    }
}
#endif
