/* ************************************************************************
> File Name:     threadPool.h
> Author:        Coyote
> Created Time:  Mon 21 Mar 2022 08:46:04 PM CST
> Description:   
 ************************************************************************/

#ifndef __THREADPOOL__
#define __THREADPOOL__

#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <queue>
#include <string>

using callback = void (*)(void*);

template<typename T>
struct Task;

template<typename T>
struct Task {
    Task<T>() : m_func(nullptr), m_arg(nullptr) {}
    Task<T>(callback func, T* arg) : m_func(func), m_arg(arg) {}

    callback m_func;
    T *m_arg;
};

template<typename T>
class TaskQueue;

template<typename T>
class TaskQueue {
public:
    TaskQueue();
    ~TaskQueue();

    void addTask(callback func, T* arg = nullptr);
    Task<T> takeTask();

    inline int getTaskNum() {
        return m_queue.size();
    }
private:
    std::queue<Task<T>> m_queue;
    pthread_mutex_t m_queue_locker;
};

struct AttrThreadPool {
    AttrThreadPool(int minNum = 2, int maxNum = 10) 
        : m_minNum(minNum), m_maxNum(maxNum), m_busyNum(0), m_aliveNum(minNum), m_exitNum(0), m_shutdown(false)
        {}
    int m_minNum;
    int m_maxNum;
    int m_busyNum;
    int m_aliveNum;
    int m_exitNum;
    bool m_shutdown;
};

template<typename T>
class ThreadPool;

template<typename T>
class ThreadPool {
public:
    ThreadPool(int minNum = 2, int maxNum = 10);
    ~ThreadPool();
    void addTask(callback func, T* arg = nullptr);

private:
    static void* manager(void*);
    static void* worker(void*);
    void threadExit();
    static const int MANAGER_THREAD_ONCENUM = 2;
private:
    AttrThreadPool m_attr;
    pthread_mutex_t m_locker;
    pthread_cond_t m_notEmpty;
    pthread_t m_managerID;
    pthread_t *m_threadsID;
    TaskQueue<T> *m_taskQueue;
};






template<typename T> 
TaskQueue<T>::TaskQueue() 
    : m_queue(std::queue<Task<T>>()) {
    if (pthread_mutex_init(&m_queue_locker, NULL) != 0) {
        throw std::exception();
    }
}

template<typename T>
void TaskQueue<T>::addTask(callback func, T* arg) {
    Task<T> task(func, arg);
    pthread_mutex_lock(&m_queue_locker);
    m_queue.push(task);
    pthread_mutex_unlock(&m_queue_locker);
}

template<typename T>
Task<T> TaskQueue<T>::takeTask() {
    Task<T> ret;
    pthread_mutex_lock(&m_queue_locker);
    if (m_queue.size() > 0) {
        ret = m_queue.front();
        m_queue.pop();
    }
    pthread_mutex_unlock(&m_queue_locker);
    return ret;
}

template<typename T>
TaskQueue<T>::~TaskQueue() {
    pthread_mutex_destroy(&m_queue_locker);
}
template<typename T>
ThreadPool<T>::ThreadPool(int minNum, int maxNum) {
    m_taskQueue = new TaskQueue<T>;
    if (!m_taskQueue) throw std::exception();

    m_threadsID = new pthread_t[maxNum];
    if (!m_threadsID) throw std::exception();

    memset(m_threadsID, 0, sizeof(pthread_t) * maxNum);
    
    if (pthread_mutex_init(&m_locker, NULL) != 0 ||
            pthread_cond_init(&m_notEmpty, NULL) != 0) {
        delete m_taskQueue;
        delete[] m_threadsID;
        throw std::exception();
    }

    pthread_create(&m_managerID, NULL, manager, this);
    for (int i = 0; i < minNum; ++i) {
        pthread_create(&m_threadsID[i], NULL, worker, this);
        std::cout << "create thread: " << std::to_string(m_threadsID[i]) << std::endl;
    }
}

template<typename T>
void* ThreadPool<T>::worker(void *arg) {
    ThreadPool *pool = static_cast<ThreadPool*>(arg);
    if (!pool) throw std::exception();
    while (1) {
        pthread_mutex_lock(&pool->m_locker);
        while (pool->m_taskQueue->getTaskNum() == 0 && !pool->m_attr.m_shutdown) {
            pthread_cond_wait(&pool->m_notEmpty, &pool->m_locker);
            if (pool->m_attr.m_exitNum > 0) {
                pool->m_attr.m_exitNum--;
                if (pool->m_attr.m_aliveNum > pool->m_attr.m_minNum) {
                    pool->m_attr.m_aliveNum--;
                    pthread_mutex_unlock(&pool->m_locker);
                    pool->threadExit();
                }
            }
        }
        
        if (pool->m_attr.m_shutdown) {
            pthread_mutex_unlock(&pool->m_locker);
            pool->threadExit();
        }

        Task<T> task = pool->m_taskQueue->takeTask();
        if (task.m_func == NULL) {
            pthread_mutex_unlock(&pool->m_locker);
            continue;
        }

        pool->m_attr.m_busyNum++;
        pthread_mutex_unlock(&pool->m_locker);
       
        std::cout << "thread " << pthread_self() << " start function...\n";
        
        task.m_func(task.m_arg);
        
        std::cout << "thread " << pthread_self() << " end function...\n";
        pthread_mutex_lock(&pool->m_locker);
        pool->m_attr.m_busyNum--;
        pthread_mutex_unlock(&pool->m_locker);
    }
    return NULL;
}

template<typename T>
void* ThreadPool<T>::manager(void *arg) {
    ThreadPool *pool = static_cast<ThreadPool*>(arg);
    if (pool == NULL) throw std::exception();
    while (!pool->m_attr.m_shutdown) {
        sleep(3);
        pthread_mutex_lock(&pool->m_locker);
        int queueSize = pool->m_taskQueue->getTaskNum();
        int liveNum = pool->m_attr.m_aliveNum;
        int busyNum = pool->m_attr.m_busyNum;
        pthread_mutex_unlock(&pool->m_locker);

        // add threads
        if (queueSize > liveNum && liveNum < pool->m_attr.m_maxNum) {
            pthread_mutex_lock(&pool->m_locker);
            int count = 0;
            for (int i = 0; i < pool->m_attr.m_maxNum && liveNum < pool->m_attr.m_maxNum && count < MANAGER_THREAD_ONCENUM; i++) {
                if (pool->m_threadsID[i] == 0) {
                    pthread_create(&pool->m_threadsID[i], NULL, worker, pool);
                    count++;
                    pool->m_attr.m_aliveNum++;
                }
            }
            pthread_mutex_unlock(&pool->m_locker);
        }
        // delete threads
        if (busyNum * 2 < liveNum && liveNum > pool->m_attr.m_minNum) {
            pthread_mutex_lock(&pool->m_locker);
            pool->m_attr.m_exitNum = MANAGER_THREAD_ONCENUM;
            pthread_mutex_unlock(&pool->m_locker);
            for (int i = 0; i < MANAGER_THREAD_ONCENUM; ++i) {
                pthread_cond_signal(&pool->m_notEmpty);
            }

        }
    }
    return NULL;
}

template<typename T>
void ThreadPool<T>::threadExit() {
    pthread_t tid = pthread_self();
    for (int i = 0; i < m_attr.m_maxNum; ++i) {
        if (m_threadsID[i] == tid) {
            m_threadsID[i] = 0;
            std::cout << "threadExit() called, " << tid << " exiting...\n";
            break;
        }
    }
    pthread_exit(NULL);
}

template<typename T>
void ThreadPool<T>::addTask(callback func, T* arg) {
    if (m_attr.m_shutdown) {
        return;
    }
    m_taskQueue->addTask(func, arg); 
    pthread_cond_signal(&m_notEmpty);
}

template<typename T>
ThreadPool<T>::~ThreadPool() {
    m_attr.m_shutdown = 1;
    pthread_join(m_managerID, NULL);
    for (int i = 0; i < m_attr.m_aliveNum; ++i) {
        pthread_cond_signal(&m_notEmpty);
    }

    if (m_taskQueue) {
        delete m_taskQueue;
        m_taskQueue = NULL;
    }

    if (m_threadsID) {
        delete[] m_threadsID;
        m_threadsID = NULL;
    }

    pthread_mutex_destroy(&m_locker);
    pthread_cond_destroy(&m_notEmpty);
}

#endif
