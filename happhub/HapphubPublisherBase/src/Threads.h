#ifdef __ANDROID
#include <pthread.h>
#endif //__ANDROID

#ifdef __WINDOWS
#include <Windows.h>
#endif //__WINDOWS

namespace happhub {

class Mutex {
friend class Condition;

public:
  Mutex();
  ~Mutex();

public:
  void lock();
  void unlock();

protected:
#ifdef __ANDROID
  pthread_mutex_t m_Mutex;
#endif //__ANDROID
#ifdef __WINDOWS
  CRITICAL_SECTION m_Mutex;
#endif //__WINDOWS
};
class MutexLocker {
public:
  MutexLocker(Mutex& mutex) :
    m_Mutex(mutex) {
    m_Mutex.lock();
  }
  ~MutexLocker() {
    m_Mutex.unlock();
  }

private:
  Mutex& m_Mutex;
};

class Condition {
public:
  Condition();
  ~Condition();

public:
  void wait(Mutex& mutex);
  void signal();

protected:
#ifdef __ANDROID
  pthread_cond_t m_Condition;
#endif //__ANDROID
#ifdef __WINDOWS
  HANDLE m_Condition;
#endif //__WINDOWS
};

class Thread {
public:
  Thread();
  ~Thread();

public:
  bool start(void* (*callback)(void* arg), void* arg);
  void join();

protected:
#ifdef __ANDROID
  pthread_t m_Thread;
#endif //__ANDROID
#ifdef __WINDOWS
  HANDLE m_Thread;
#endif //__WINDOWS
};

}
