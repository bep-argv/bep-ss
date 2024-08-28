/* control.h -- General pthread lock control class */
#ifndef CONTROL_H
#define CONTROL_H

#include <pthread.h>

class thread_ctl {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  int active;
 public:
  int init();
  int destroy();
  int activate();
  int deactivate();
  int lock();
  int unlock();
  int cond_wait();
  int cond_signal();
  int cond_broadcast();
  int isactive();
};

#endif
