/* ************************************************************************
> File Name:     httpconn.cpp
> Author:        Coyote
> Created Time:  Thu 10 Mar 2022 04:31:37 PM CST
> Description:   
 ************************************************************************/
#include "httpconn.h"

int Http_Conn::m_epollfd = -1;

void Http_Conn::init(int sockfd) {
    m_connfd = sockfd;
    init();
}

void Http_Conn::init() {
    std::ifstream fin(httpConfig);
    nlohmann::json config_json;
    fin >> config_json;

    m_index_html = config_json.at("index");
    m_url = 0; 
    m_version = 0;
    m_check_state = CHECK_STATE_REQUESTLINE;
    METHOD m_method = GET;        
    cgi = config_json.at("cgi");               
    m_content_length = 0;  
    m_host = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_connection = config_json.at("connection");      
    m_user = 0;           
    
    memset(m_read_buf, '\0', READ_BUFF_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFF_SIZE);

}

bool Http_Conn::recv_msg() {
    if (m_read_idx > READ_BUFF_SIZE) return false;
    int bytes_read = 0;
    bytes_read = recv(m_connfd, m_read_buf + m_read_idx, READ_BUFF_SIZE - m_read_idx, 0);
    m_read_idx += bytes_read;

    if (bytes_read <= 0)
    {
        return false;
    }

    return true;
}

Http_Conn::HTTP_CODE Http_Conn::read_handle() {
    recv_msg();
    LINE_STATUS ls = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = 0;
    
    while ((ls = parse_line()) == LINE_OK) {
        text = m_read_buf + m_start_line;
        m_start_line = m_checked_idx;
        switch(m_check_state) {
            case CHECK_STATE_REQUESTLINE:
                {
                    ret = parse_request_line(text);
                    if (ret == BAD_REQUEST) return ret;
                    break;
                }
            case CHECK_STATE_HEADER:
                {
                    ret = parse_headers(text);
                    if (ret == BAD_REQUEST) return ret;
                    else if (ret == GET_REQUEST) {
                        return ret;
                    }
                    break;
                }
            case CHECK_STATE_CONTENT:
                {
                    ret = parse_content(text);
                    if (ret == GET_REQUEST) {
                        return ret;
                    }
                    ls = LINE_OPEN;
                    break;
                }
            default:
                return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

Http_Conn::HTTP_CODE Http_Conn::request_handle() {
    if (m_method == GET) {
        send_file(m_index_html.c_str());
        return FILE_REQUEST;
    } else {

    }
    return GET_REQUEST;
}

Http_Conn::LINE_STATUS Http_Conn::parse_line()
{
    char temp;
    for (; m_checked_idx < m_read_idx; ++m_checked_idx)
    {
        temp = m_read_buf[m_checked_idx];
        if (temp == '\r')
        {
            if ((m_checked_idx + 1) == m_read_idx)
                return LINE_OPEN;
            else if (m_read_buf[m_checked_idx + 1] == '\n')
            {
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if (temp == '\n')
        {
            if (m_checked_idx > 1 && m_read_buf[m_checked_idx - 1] == '\r')
            {
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

//???????????????
Http_Conn::HTTP_CODE Http_Conn::parse_request_line(char *text) {
    m_url = strpbrk(text, " \t");
    if (!m_url) {
        return BAD_REQUEST;
    }

    *m_url++ = '\0';
    char *method = text;
    if (strcasecmp(method, "GET") == 0) {
        m_method = GET;
    } else if (strcasecmp(method, "POST") == 0) {
        m_method = POST;
        cgi = 1;
    } else {
        return BAD_REQUEST;
    }
    
    m_url += strspn(m_url, " \t");
    m_version = strpbrk(m_url, " \t");
    if (!m_version) {
        return BAD_REQUEST;
    }
     *m_version++ = '\0';
    m_version += strspn(m_version, " \t");
    if (strcasecmp(m_version, "HTTP/1.1") != 0)
        return BAD_REQUEST;
    if (strncasecmp(m_url, "http://", 7) == 0)
    {
        m_url += 7;
        m_url = strchr(m_url, '/');
    }

    if (strncasecmp(m_url, "https://", 8) == 0)
    {
        m_url += 8;
        m_url = strchr(m_url, '/');
    }
    if (!m_url || m_url[0] != '/')
        return BAD_REQUEST;
    //???url???/??????????????????
    if (strlen(m_url) == 1)
        strcat(m_url, "index.html");
    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

//??????http?????????????????????
Http_Conn::HTTP_CODE Http_Conn::parse_headers(char *text)
{
    if (text[0] == '\0')
    {
        if (m_content_length != 0)
        {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    else if (strncasecmp(text, "Connection:", 11) == 0)
    {
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0)
        {
            m_connection = true;
        }
    }
    else if (strncasecmp(text, "Content-length:", 15) == 0)
    {
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atol(text);
    }
    else if (strncasecmp(text, "Host:", 5) == 0)
    {
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    }
    else
    {
        /* LOG_INFO("oop!unknow header: %s", text); */
    }
    return NO_REQUEST;
}

//??????http???????????????????????????
Http_Conn::HTTP_CODE Http_Conn::parse_content(char *text)
{
    if (m_read_idx >= (m_content_length + m_checked_idx))
    {
        text[m_content_length] = '\0';
        //POST?????????????????????????????????????????????
        m_user = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

ssize_t Http_Conn::send_file(const char *filename)
{
    int filefd = open(filename, O_RDONLY);
    if (filefd <= 0) {
        std::cout << "cannot open file: " << filename << std::endl;
        return -1;
    }

    struct stat stat_buf;
    fstat(filefd, &stat_buf);

    char header[HEADER_LENGTH];
    http_header_get_html(header);

    if (send(m_connfd, header, strlen(header), 0) == -1) {
        return -1;
    }

    sendfile(m_connfd, filefd, NULL, stat_buf.st_size);
    modfd(m_epollfd, m_connfd, EPOLLIN);
    return 0;
}

void Http_Conn::http_header_get_html(char *header) {
    snprintf(header, HEADER_LENGTH - 1, "HTTP/1.1 200 OK\r\nContent-Type: text/html;charset=utf-8\r\nContent-length: 1024\r\nServer: Coyote's Server 0.1\r\n\r\n");
}

void Http_Conn::modfd(int epollfd, int connfd, int ev) {
    epoll_event event;
    event.data.fd = connfd;
    event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, connfd, &event);
}

void Http_Conn::read_handler(void* var) {
    Http_Conn *obj = (Http_Conn*)var;
    HTTP_CODE ret = obj->read_handle();
    if (ret == NO_REQUEST) {
        obj->modfd(m_epollfd, obj->m_connfd, EPOLLIN);
        return;
    }
    obj->modfd(m_epollfd, obj->m_connfd, EPOLLOUT);
}

void Http_Conn::write_handler(void* var) {
    Http_Conn *obj = (Http_Conn*)var;
    obj->request_handle();
    epoll_ctl(m_epollfd, EPOLL_CTL_DEL, obj->m_connfd, 0);
    close(obj->m_connfd);
}
