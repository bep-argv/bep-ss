#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/soundcard.h>
#include <string>
#include "config.h"
#include "ssdefs.h"
#include "control.h"
#include "dsp.h"

ssDspDevice::ssDspDevice()
{
  fd = -1;
  ctl.init();
  devLock.init();
  current = NULL;
  devName = "";
}

ssDspDevice::ssDspDevice(char *name)
{
  fd = -1;
  ctl.init();
  devLock.init();
  current = NULL;
  setDevName(name);
#ifdef DEBUG
  fprintf(stderr, "DSP: Using sound device: %s\n", devName.c_str());
#endif
  sd = new soundGenerator(SS_DEFAULT_SAMPLES_PER_SEC, SS_TONE_VOLUME, SS_PCMSIZE);
}

ssDspDevice::~ssDspDevice()
{
  devLock.lock();
  delete sd;
  if (fd != -1) {
    close();
  }
  devLock.unlock();
  devLock.destroy();
  ctl.destroy();
}

int
ssDspDevice::write(const void *buff, size_t count)
{
  if (fd == -1) {
    return 0;
  }
  devLock.lock();
  int iRet = ::write(fd, buff, count);
  devLock.unlock();
  return iRet;
}

int
ssDspDevice::finish()
{
  audio_buf_info info;
  if (fd == -1) {
    return 0;
  }
  devLock.lock();
  ioctl(fd, SNDCTL_DSP_GETOSPACE, &info);
  int remain = info.bytes;
#ifdef DEBUG
  fprintf(stderr, "DSP: Padding: remain buf %d\n", remain);
#endif
  if (info.fragments != info.fragstotal) {
    char dumbuf[remain];
    memset(dumbuf, 0, remain);
    ::write(fd, dumbuf, remain);
  }
  devLock.unlock();
  return remain;
}

int
ssDspDevice::reset()
{
  int iRet = 0;
  if (fd == -1)
    return -1;
  devLock.lock();
  iRet = ioctl (fd, SNDCTL_DSP_RESET);
	::close(fd);
  fd = -1;
	devLock.unlock();
  return iRet;
}

int
ssDspDevice::sync()
{
  int iRet = 0;
  if (fd == -1)
    return -1;
  devLock.lock();
  iRet = ioctl (fd, SNDCTL_DSP_SYNC);
  devLock.unlock();
  return iRet;
}

int
ssDspDevice::open()
{
  int stereo = FALSE;
  int fmts = AFMT_S16_LE;
  int dsp_speed = SS_DEFAULT_SAMPLES_PER_SEC;
  if (fd != -1 || devName == "") {
    return fd;
  }
  devLock.lock();
  fd = ::open(devName.c_str(), O_WRONLY);
  if (fd==-1) {
    devLock.unlock();
    return -1;
  }
  ioctl (fd, SNDCTL_DSP_RESET);
  ioctl (fd, SNDCTL_DSP_SPEED, &dsp_speed);
  ioctl (fd, SNDCTL_DSP_STEREO, &stereo);
  ioctl (fd, SNDCTL_DSP_SETFMT, &fmts);
  int bufSize = 0x00240008;  /* 256 * 24 bytes */
  ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &bufSize);
  ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &bufSize);
#ifdef DEBUG
  fprintf(stderr, "DSP: bufsize: %d\n", bufSize);
#endif /* DEBUG */
  devLock.unlock();
  return fd;
}

int
ssDspDevice::close()
{
  if (fd == -1)
    return 0;
  devLock.lock();
  int iRet = ::close(fd);
  fd = -1;
  devLock.unlock();
  return iRet;
}

void
ssDspDevice::setDevName(char *name)
{
  devName = name;
}

void
ssDspDevice::reserveLock(void *const obj)
{
  ctl.lock();
  lockSeq.push(obj);
  ctl.unlock();
  ctl.cond_signal();
#if DEBUG >= 3 && WRITE_ELOG
  log_error(NULL, "dsp: reserved by %d\n", (unsigned int)obj);
#endif
}

void
ssDspDevice::waitLock(void *const obj)
{
  ctl.lock();
  while(lockSeq.gethead() != obj) {
    ctl.cond_wait();
  }
  ctl.unlock();
  ctl.cond_signal();
#if DEBUG >= 3 && WRITE_ELOG
  log_error(NULL, "dsp: used by %d\n", (unsigned int)obj);
#endif
  return;
}

void
ssDspDevice::releaseLock(void *obj)
{
  ctl.lock();
  //while(lockSeq.gethead() != obj)
  //  ctl.cond_wait();
  void *obj0 = lockSeq.pop();
#if 0
  if (lockSeq.empty()) {
    close();
  }
#endif
  ctl.unlock();
  ctl.cond_signal();
#if DEBUG >= 3 && WRITE_ELOG
  log_error(NULL, "dsp: released by %d real: %d\n", (unsigned int)obj,
	    (unsigned int)obj0);
#endif
  if (obj != obj0) {
    fprintf(stderr, "dsp: Warning: lock is released by the other object\n");
#ifdef WRITE_ELOG
    log_error(NULL, "dsp: Warning: lock is released by the other object\n");
#endif
  }
  return;
}

void
ssDspDevice::clearLock()
{
  ctl.lock();
  lockSeq.clear_without_content();
  ctl.unlock();
  return;
}

void
ssDspDevice::clearLock(void * obj)
{
  int mark; /* We need a pointer */
  ctl.lock();
  void *tts;
  /* push a fake pointer as a mark. */
  lockSeq.push(&mark);
  while(lockSeq.gethead() != &mark) {
    tts = lockSeq.pop();
    /* remove queue entry which contains obj. */
    if (tts != obj)
      lockSeq.push(tts);
  }
  lockSeq.pop();
  ctl.unlock();
  return;
}

int
ssDspDevice::playTone(const char *cmd_args)
{
  /* 引数の文字列を周波数と長さの組に変換 */
  int fr, ms;
  char args[strlen(cmd_args) + 1];
  char *args2;
  strcpy(args, cmd_args);
  char *p = args;
  while (isdigit(*p))
    p++;
  *p = '\0';
  fr = atoi(args);
#if DEBUG >= 3
  fprintf(stderr, "fr: %d, ", fr);
#endif
  args2 = ++p;;
  while (!isdigit(*args2) && *args2 != '\0')
    args2++;
  if (*args2 == '\0')
    return 0;
  p = args2;
  while (isdigit(*p))
    p++;
  *p = '\0';
  ms = atoi(args2);
#if DEBUG >= 3
  fprintf(stderr, "ms: %d\n", ms);
#endif
  /* コマンドに従って音を出力 */
  int ret;
  char buf[SS_PCMSIZE];
  this->open();
  do {
    ret = sd->generate(fr, ms, buf);
    this->write(buf, ret);
  } while(ret == SS_PCMSIZE);
  memset(buf, 0, SS_PCMSIZE);
  this->write(buf, SS_PCMSIZE);
  sd->init();
  return 0;
}
