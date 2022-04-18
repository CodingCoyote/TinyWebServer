/* ************************************************************************
> File Name:     sqlConn.h
> Author:        Coyote
> Created Time:  Wed 13 Apr 2022 08:18:52 PM CST
> Description:   
 ************************************************************************/

#include <list>
#include <iostream>
#include <string>
#include <mysql/mysql.h>
#include <pthread.h>
#include <semaphore.h>

class SqlPool {
public:
    static SqlPool* getInstance();
public:
    std::string m_host;
    int m_port;
    std::string m_username;
    std::string m_password;
    std::string m_db_name;
public:
    MYSQL* getConnection();
    void releaseConnection(MYSQL*);
    inline int getFreeNum() {
        return m_freeNum;
    }
    void init(std::string host, int port, std::string db_name, std::string username, std::string password, int maxNum);
private:
    SqlPool() : m_freeNum(0), m_usedNum(0) {}
    ~SqlPool();
    int m_freeNum;
    int m_maxNum;
    int m_usedNum;
private:
    pthread_mutex_t m_mutex;
    std::list<MYSQL*> m_list;
    sem_t m_sem;
};

class SqlConnWrapped {
public:
    SqlConnWrapped(MYSQL** con, SqlPool* pool);
    ~SqlConnWrapped();
private:
    MYSQL *m_conn;
    SqlPool *m_pool;
};
