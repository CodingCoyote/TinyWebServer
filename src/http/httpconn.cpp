/* ************************************************************************
> File Name:     httpconn.cpp
> Author:        Coyote
> Created Time:  Thu 10 Mar 2022 04:31:37 PM CST
> Description:   
 ************************************************************************/
#include "httpconn.h"

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
                    else if (ret == GET_REQUEST) return request_handle();
                    break;
                }
            case CHECK_STATE_CONTENT:
                {
                    ret = parse_content(text);
                    if (ret == GET_REQUEST) return request_handle();
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

//解析请求行
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
    //当url为/时，显示主页
    if (strlen(m_url) == 1)
        strcat(m_url, "index.html");
    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

//解析http请求的头部信息
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

//判断http请求是否被完整读入
Http_Conn::HTTP_CODE Http_Conn::parse_content(char *text)
{
    if (m_read_idx >= (m_content_length + m_checked_idx))
    {
        text[m_content_length] = '\0';
        //POST请求中最后为输入的用户名和密码
        m_user = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

ssize_t Http_Conn::send_file(const char *filename)
{
    std::ifstream fin(filename);
    if (!fin.is_open()) {
        std::cout << "cannot open file: " << filename << std::endl;
        return -1;
    }

    char header[HEADER_LENGTH];
    http_header_get_html(header);

    if (send(m_connfd, header, strlen(header), 0) == -1) {
        return -1;
    }

    std::string tmp;
    while (getline(fin, tmp)) {
        if (send(m_connfd, tmp.c_str(), tmp.size(), 0) == -1) {
            std::cout << "send_file error\n";
            return -1;
        }
    }

    fin.close();
    return 0;
}

void Http_Conn::http_header_get_html(char *header)
{
    snprintf(header, HEADER_LENGTH - 1, "HTTP/1.1 200 OK\r\nContent-Type: text/html;charset=utf-8\r\nContent-length: 1024\r\nServer: Coyote's Server 0.1\r\n\r\n");
}

void Http_Conn::handler(void* var)
{
    Http_Conn *obj = (Http_Conn*)var;
    obj->read_handle(); 
    close(obj->m_connfd);
}

