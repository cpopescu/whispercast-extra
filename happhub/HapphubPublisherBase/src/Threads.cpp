#include "Threads.h"

namespace happhub {

// __ANDROID
#ifdef __ANDROID

Mutex::Mutex() {
  pthread_mutex_init(&m_Mutex, NULL);
}
Mutex::~Mutex() {
  pthread_mutex_destroy(&m_Mutex);
}

void Mutex::lock() {
  pthread_mutex_lock(&m_Mutex);
}
void Mutex::unlock() {
  pthread_mutex_unlock(&m_Mutex);
}

Condition::Condition() {
  pthread_cond_init(&m_Condition, NULL);
}
Condition::~Condition() {
  pthread_cond_destroy(&m_Condition);
}

void Condition::wait(Mutex& mutex) {
  pthread_cond_wait(&m_Condition, &mutex.m_Mutex);
}
void Condition::signal() {
  pthread_cond_signal(&m_Condition);
}

Thread::Thread() {
}
Thread::~Thread() {
}

bool Thread::start(void* (*callback)(void* arg), void* arg) {
  if (pthread_create(&m_Thread, NULL, callback, arg) == 0) {
    return true;
  }
  return false;
}
void Thread::join() {
  pthread_join(m_Thread, NULL);
}

#endif //__ANDROID

// __WINDOWS
#ifdef __WINDOWS

Mutex::Mutex() {
  ::InitializeCriticalSection(&m_Mutex);
}
Mutex::~Mutex() {
  ::DeleteCriticalSection(&m_Mutex);
}

void Mutex::lock() {
  ::EnterCriticalSection(&m_Mutex);
}
void Mutex::unlock() {
  ::LeaveCriticalSection(&m_Mutex);
}

Condition::Condition() {
  m_Condition = ::CreateEvent(NULL, FALSE, FALSE, NULL);
}
Condition::~Condition() {
  ::CloseHandle(m_Condition);
}

void Condition::wait(Mutex& mutex) {
  mutex.unlock();
  ::WaitForSingleObject(m_Condition, INFINITE);
  mutex.lock();
}
void Condition::signal() {
  ::SetEvent(m_Condition);
}

Thread::Thread() :
  m_Thread(NULL) {
}
Thread::~Thread() {
}

bool Thread::start(void* (*callback)(void* arg), void* arg) {
  m_Thread = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)callback, arg, 0, NULL);
  return (m_Thread != NULL);
}
void Thread::join() {
  ::WaitForSingleObject(m_Thread, INFINITE);

  ::CloseHandle(m_Thread);
  m_Thread = NULL;
}

#endif // __WINDOWS

}
