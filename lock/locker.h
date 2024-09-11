#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>
#include <iostream>
#include <assert.h>

/* 断言处理 */
static void CondPanic(bool condition, std::string err)
{
    if(!condition)
    {
        std::cout << "[assert by] (" << __FILE__ << ":" << __LINE__ << "), err: " << err << std::endl;
        assert(condition);
    }
}

class Nonecopyable
{
public:
    Nonecopyable() = default;
    ~Nonecopyable() = default;
    Nonecopyable(const Nonecopyable&) = delete;
    Nonecopyable operator=(const Nonecopyable) = delete;
};

class Semaphore //: Nonecopyable
{
public:
    Semaphore(uint32_t count = 0)
    {
        CondPanic(0 == sem_init(&semaphore_, 0, count), "sem init error");
    }

    ~Semaphore()
    {
        CondPanic(0 == sem_destroy(&semaphore_), "sem destroy error");
    }

    void wait()
    {
        CondPanic(0 == sem_wait(&semaphore_), "sem wait error");
    }

    void notify()
    {
        CondPanic(0 == sem_post(&semaphore_), "sem post error");
    }

private:
    sem_t semaphore_;
};

/**
 * @brief 局部锁 类模板
 */
template <class T>
struct ScopedLockImpl
{
    ScopedLockImpl(T &mutex) : m_(mutex)
    {
        m_.lock();
        isLocked_ = true;
    }

    void lock()
    {
        if(!isLocked_)
        {
            m_.lock();
            isLocked_ = true;
        }
    }

    void unlock()
    {
        if(isLocked_)
        {
            m_.unlock();
            isLocked_ = false;
        }
    }

    ~ScopedLockImpl()
    {
        unlock();
    }

private:
    T &m_;  //mutex
    bool isLocked_; //是否已经上锁
};

template<class T>
struct ReadScopedLockImpl
{
public:
    ReadScopedLockImpl(T &mutex) : mutex_(mutex)
    {
        mutex_.rdlock();
        isLocked_ = true;
    }

    ~ReadScopedLockImpl()
    {
        unlock();
    }

    void lock()
    {
        if(!isLocked_)
        {
            mutex_.rdlock();
            isLocked_ = true;
        }
    }

    void unlock()
    {
        if(isLocked_)
        {
            mutex_.unlock();
            isLocked_ = false;
        }
    }

private:
    T &mutex_;
    bool isLocked_;
};

template <class T>
struct WriteScopedLockImpl {
public:
    WriteScopedLockImpl(T &mutex) : mutex_(mutex)
    {
        mutex_.wrlock();
        isLocked_ = true;
    }

    ~WriteScopedLockImpl() 
    { 
        unlock();
    }

    void lock()
    {
        if (!isLocked_)
        {
            mutex_.wrlock();
            isLocked_ = true;
        }
    }

    void unlock()
    {
        if (isLocked_)
        {
            mutex_.unlock();
            isLocked_ = false;
        }
    }

private:
    T &mutex_;
    bool isLocked_;
};

class Mutex : Nonecopyable
{
public:
    typedef ScopedLockImpl<Mutex> Lock;

    Mutex()
    {
        CondPanic(0 == pthread_mutex_init(&m_, nullptr), "lcok init error");
    }

    ~Mutex()
    {
        CondPanic(0 == pthread_mutex_destroy(&m_), "destroy lock error");
    }

    void lock()
    {
        CondPanic(0 == pthread_mutex_lock(&m_), "lock error");
    }

    void unlock()
    {
        CondPanic(0 == pthread_mutex_unlock(&m_), "unlock error");
    }

    friend class CondVar;

private:
    pthread_mutex_t m_;
};

class CondVar;
class RWMutex : Nonecopyable
{
public:
    /* 局部读写锁 */
    typedef ReadScopedLockImpl<RWMutex> ReadLock;

    /* 局部写锁 */
    typedef WriteScopedLockImpl<RWMutex> WriteLock;

    RWMutex()
    {
        pthread_rwlock_init(&m_, nullptr);
    }

    ~RWMutex()
    {
        pthread_rwlock_destroy(&m_);
    }

    void rdlock()
    {
        pthread_rwlock_rdlock(&m_);
    }

    void wrlock()
    {
        pthread_rwlock_wrlock(&m_);
    }

    void unlock()
    {
        pthread_rwlock_unlock(&m_);
    }

private:
    pthread_rwlock_t m_;
};

class CondVar : Nonecopyable
{
public:
    CondVar()
    {
        CondPanic(0 == pthread_cond_init(&cond_, nullptr), "CondVar init error");
    }

    ~CondVar()
    {
        CondPanic(0 == pthread_cond_destroy(&cond_), "CondVar destroy error");
    }

    void wait(Mutex &mutex)
    {
        CondPanic(0 == pthread_cond_wait(&cond_, &mutex.m_), "CondVar wait error");
    }

    void timewait(Mutex &mutex, struct timespec t)
    {
        CondPanic(0 == pthread_cond_timedwait(&cond_, &mutex.m_, &t), "CondVar wait error");
    }

    void signal()
    {
        CondPanic(0 == pthread_cond_signal(&cond_), "CondVar signal error");
    }

    void broadcast()
    {
        CondPanic(0 == pthread_cond_broadcast(&cond_), "CondVar broadcast error");
    }

private:
    pthread_cond_t cond_;
};





// class sem
// {
// public:
//     sem()
//     {
//         if (sem_init(&m_sem, 0, 0) != 0)
//         {
//             throw std::exception();
//         }
//     }
//     sem(int num)
//     {
//         if (sem_init(&m_sem, 0, num) != 0)
//         {
//             throw std::exception();
//         }
//     }
//     ~sem()
//     {
//         sem_destroy(&m_sem);
//     }
//     bool wait()
//     {
//         return sem_wait(&m_sem) == 0;
//     }
//     bool post()
//     {
//         return sem_post(&m_sem) == 0;
//     }

// private:
//     sem_t m_sem;
// };

// class locker
// {
// public:
//     locker()
//     {
//         if (pthread_mutex_init(&m_mutex, NULL) != 0)
//         {
//             throw std::exception();
//         }
//     }
//     ~locker()
//     {
//         pthread_mutex_destroy(&m_mutex);
//     }
//     bool lock()
//     {
//         return pthread_mutex_lock(&m_mutex) == 0;
//     }
//     bool unlock()
//     {
//         return pthread_mutex_unlock(&m_mutex) == 0;
//     }
//     pthread_mutex_t *get()
//     {
//         return &m_mutex;
//     }

// private:
//     pthread_mutex_t m_mutex;
// };
// class cond
// {
// public:
//     cond()
//     {
//         if (pthread_cond_init(&m_cond, NULL) != 0)
//         {
//             //pthread_mutex_destroy(&m_mutex);
//             throw std::exception();
//         }
//     }
//     ~cond()
//     {
//         pthread_cond_destroy(&m_cond);
//     }
//     bool wait(pthread_mutex_t *m_mutex)
//     {
//         int ret = 0;
//         //pthread_mutex_lock(&m_mutex);
//         ret = pthread_cond_wait(&m_cond, m_mutex);
//         //pthread_mutex_unlock(&m_mutex);
//         return ret == 0;
//     }
//     bool timewait(pthread_mutex_t *m_mutex, struct timespec t)
//     {
//         int ret = 0;
//         //pthread_mutex_lock(&m_mutex);
//         ret = pthread_cond_timedwait(&m_cond, m_mutex, &t);
//         //pthread_mutex_unlock(&m_mutex);
//         return ret == 0;
//     }
//     bool signal()
//     {
//         return pthread_cond_signal(&m_cond) == 0;
//     }
//     bool broadcast()
//     {
//         return pthread_cond_broadcast(&m_cond) == 0;
//     }

// private:
//     //static pthread_mutex_t m_mutex;
//     pthread_cond_t m_cond;
// };


#endif
