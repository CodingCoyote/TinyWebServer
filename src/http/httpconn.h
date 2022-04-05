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
    static void handler(void*);
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
private:
    const char *httpConfig = "../config/httpConfig.json";
    int m_connfd;           //客户端连接fd
    char *m_url;            //url
    char *m_version;        //http_version
    CHECK_STATE m_check_state;//主状态机
    METHOD m_method;        //从状态机
    uint32_t cgi;               //是否启用cgi
    uint32_t m_content_length;  //内容长度
    char *m_host;
    uint32_t m_checked_idx;
    uint32_t m_read_idx;
    bool m_connection;      //http长连接
    char *m_user;           //登录用户
    std::string m_index_html;           //主页 
    char m_read_buf[READ_BUFF_SIZE];    //读缓冲区
    char m_write_buf[WRITE_BUFF_SIZE];  //写缓冲区
    int m_start_line;
};

#endif
