/* ************************************************************************
> File Name:     src/httpconn.h
> Author:        Coyote
> Created Time:  Thu 10 Mar 2022 04:14:11 PM CST
> Description:   
 ************************************************************************/
#ifndef __HTTPCONN_H__
#define __HTTPCONN_H__

#include <strings.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "nlohmann/json.hpp"
#include <fstream>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <sys/epoll.h>

class Http_Conn {
public:
    Http_Conn() {}
    ~Http_Conn() {}
public:
    // static const int BUFF_SIZE = 1024;
    static const int READ_BUFF_SIZE = 2048;
    static const int WRITE_BUFF_SIZE = 1024;
    static const int HEADER_LENGTH = 1024;
    enum METHOD {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

public:
    static void read_handler(void*);
    static void write_handler(void*);
    static int m_epollfd;
public:
    void init();
    void init(int sockfd);
    HTTP_CODE read_handle();
    HTTP_CODE request_handle();
    LINE_STATUS parse_line();
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    ssize_t send_file(const char *filename);
    void http_header_get_html(char *header);
    bool recv_msg();
    void modfd(int, int, int);
private:
    const char *httpConfig = "../config/httpConfig.json";
    int m_connfd;           //???????????????fd
    char *m_url;            //url
    char *m_version;        //http_version
    CHECK_STATE m_check_state;//????????????
    METHOD m_method;        //????????????
    uint32_t cgi;               //????????????cgi
    uint32_t m_content_length;  //????????????
    char *m_host;
    uint32_t m_checked_idx;
    uint32_t m_read_idx;
    bool m_connection;      //http?????????
    char *m_user;           //????????????
    std::string m_index_html;           //?????? 
    char m_read_buf[READ_BUFF_SIZE];    //????????????
    char m_write_buf[WRITE_BUFF_SIZE];  //????????????
    int m_start_line;
};

#endif
