/* control.cpp -- General thread lock control class */

#include "control.h"

int
thread_ctl::init()
{
  if (pthread_mutex_init(&mutex,NULL))
    return 1;
  if (pthread_cond_init(&cond,NULL))
    return 2;
  active=0;
  return 0;
}

int
thread_ctl::destroy()
{
  if (pthread_cond_destroy(&cond))
    return 1;
  if (pthread_cond_destroy(&cond))
    return 2;
  active=0;
  return 0;
}

int
thread_ctl::activate()
{
  if (pthread_mutex_lock(&mutex))
    return 0;
  active=1;
  pthread_mutex_unlock(&mutex);
  pthread_cond_broadcast(&cond);
  return 1;
}

int
thread_ctl::deactivate()
{
  if (pthread_mutex_lock(&mutex))
    return 0;
  active=0;
  pthread_mutex_unlock(&mutex);
  pthread_cond_broadcast(&cond);
  return 1;
}

int
thread_ctl::lock()
{
  return pthread_mutex_lock(&mutex);
}

int
thread_ctl::unlock()
{
  return pthread_mutex_unlock(&mutex);
}

int
thread_ctl::cond_wait()
{
  return pthread_cond_wait(&cond, &mutex);
}

int
thread_ctl::cond_signal()
{
  return pthread_cond_signal(&cond);
}

int
thread_ctl::cond_broadcast()
{
  return pthread_cond_broadcast(&cond);
}

/* Don't lock the mutex and cond. Use this only if you are assure that
 you have got the lock. */
int
thread_ctl::isactive()
{
  return active;
}
