#ifndef DSP_H
#define DSP_H

#include <stdio.h>
#include <unistd.h>
#include <string>
#include "control.h"
#include "queue.h"
#include "soundgen.h"

class ssDspDevice {
  thread_ctl ctl;
  thread_ctl devLock;
  void *current;
  int fd;
  string devName;
  queue lockSeq;
  soundGenerator *sd;
 public:
  ssDspDevice();
  ssDspDevice(char *);
  ~ssDspDevice();
  int write(const void *, size_t);
  int finish();
  int reset();
  int sync();
  int open();
  int close();
  void setDevName(char *);
  void reserveLock(void *const);
  void waitLock(void *const );
  void releaseLock(void *);
  void clearLock();
  void clearLock(void *);
  int playTone(const char *cmd_args);
};

#endif
