/* ************************************************************************
> File Name:     sqlConn.cpp
> Author:        Coyote
> Created Time:  Thu 14 Apr 2022 02:29:01 PM CST
> Description:   
 ************************************************************************/

#include "sqlConn.h"

SqlPool* SqlPool::getInstance() {
    static SqlPool pool;
    return &pool;
}

SqlPool::~SqlPool() {
   pthread_mutex_lock(&m_mutex);
   if (m_list.size() > 0) {
       auto it = m_list.begin();
       while (it != m_list.end()) {
           MYSQL *con = *it;
           mysql_close(con);
           it++;
       }
       m_freeNum = 0;
       m_usedNum = 0;
       m_list.clear();
   }
   pthread_mutex_unlock(&m_mutex);
}

void SqlPool::init(std::string host, int port, std::string db_name, std::string username, std::string password, int maxNum) {
    m_host = host;
    m_port = port;
    m_db_name = db_name;
    m_username = username;
    m_password = password;
    m_maxNum = maxNum;

    for (int i = 0; i < m_maxNum; ++i) {
        MYSQL *con = NULL;
        con = mysql_init(con);
        if (con == NULL) {
            std::cout << "mysql init error\n";
            exit(1);
        }
        con = mysql_real_connect(con, m_host.c_str(), m_username.c_str(), m_password.c_str(), m_db_name.c_str(), m_port, NULL, 0);
        if (con == NULL) {
            std::cout << "mysql init error\n";
            exit(1);
        }
        m_list.push_back(con);
        ++m_freeNum;
    }

    sem_init(&m_sem, 0, m_freeNum);
    m_maxNum = m_freeNum;

}

MYSQL* SqlPool::getConnection() {
    MYSQL *con = NULL;
    if (m_list.size() == 0) return NULL;

    sem_wait(&m_sem);
    pthread_mutex_lock(&m_mutex);

    con = m_list.front();
    m_list.pop_front();
    ++m_usedNum;
    --m_freeNum;
    pthread_mutex_unlock(&m_mutex);
    return con;
}

void SqlPool::releaseConnection(MYSQL* con) {
    if (con == NULL) return;
    pthread_mutex_lock(&m_mutex);
    m_list.push_back(con);
    ++m_freeNum;
    --m_usedNum;
    sem_post(&m_sem);
    pthread_mutex_unlock(&m_mutex);
}

SqlConnWrapped::SqlConnWrapped(MYSQL** con, SqlPool* pool) {
    *con = m_pool->getConnection();
    m_conn = *con;
    m_pool = pool;
}

SqlConnWrapped::~SqlConnWrapped() {
    m_pool->releaseConnection(m_conn);
}
