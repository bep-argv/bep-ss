/*
** ssfunc.cpp --- Utility functions
**
** Copyright (C) 2000, 2001 Bilingual Emacspeak Project <inoue@argv.org>
**
** Last modified: $Date: 2002/01/15 11:21:42 $ by $Author: seiken $
** Keywords: Emacs, Emacspeak, speech, Linux
**
** This file is part of BEP (Bilingual Emacspeak Project) 
** <http://www.argv.org/bep/>
** This file is originally written by Gary Bishop, and modified by BEP
** to make a bilingual, Japanese and English, DECTalk-like speech server 
** running under Windows.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/soundcard.h>
#include <pthread.h>
#include <string>
#include "ssdefs.h"
// #include "dic.h"
// #include "dt.h"
#include "spcsrv.h"
#if SS_SYN_USE_ESD==TRUE
#include <esd.h>
#endif

char *ss_read_input(FILE *fp)
{
  static char *buf = NULL;
  static int buf_blocks;
  int pos;
  int c;
  
  if (buf == NULL) {
    buf = (char *)malloc(SS_INPUT_BLK_SIZE * sizeof(char));
    if (buf == NULL) {
      return NULL;
    }
    buf_blocks = 1;
  }
  pos = 0;
  while ((c = fgetc(fp)) != EOF) {
    buf[pos++] = c;
    if (pos == SS_INPUT_BLK_SIZE * buf_blocks) {
      buf = (char *)realloc(buf,
				     SS_INPUT_BLK_SIZE * (++buf_blocks));
    }
    if (c == '\n') {
      buf[pos-1] = '\0';
      return buf;
    }
  }
  free (buf);
  return NULL;
}

#if SS_SYN_USE_ESD==TRUE
void
ss_make_prog_name(char *namep, int len)
{
  char hostname[MAXHOSTNAMELEN];
  char pidStr[10];
  int pid;
  int iRet;

  iRet = gethostname(hostname, MAXHOSTNAMELEN);
  pid = getpid();
  sprintf(pidStr, "%d", pid);
  if ((int)(strlen(hostname) + strlen(pidStr)) < len - 2) {
    strcpy(namep, hostname);
    strcat(namep, pidStr);
  }
  printf("get prog_name: %s\n", namep);
  return;
}

int open_audio(char *host, char *prog_namep)
{
  int sock = -1;
  int rate = 16000;
  int bits = ESD_BITS16, channels = ESD_MONO;
  int mode = ESD_STREAM;
  int func = ESD_PLAY;
  esd_format_t format = 0;
  char prog_name[SS_MAX_PROG_NAME_LEN];

  if (host == NULL) {
    host = getenv(SS_SYN_HOST_ENV);
  }
  strcpy(prog_name, SS_SYN_PROG_NAME);
  ss_make_prog_name(prog_name, SS_MAX_PROG_NAME_LEN);
  if (prog_namep != NULL) {
    strcpy(prog_namep, prog_name);
  }

  format = bits | channels | mode | func;
  sock = esd_play_stream_fallback( format, rate, host, prog_name );
  return sock;
}
#else
int open_audio(char *audevice)
{
  int fd;
  /*int dsp_samplesize = 16;*/
  int stereo=FALSE;
  int fmts=AFMT_S16_LE;
  int dsp_speed=16000;
  fd=open(audevice, O_WRONLY);
  if (fd==-1) {
    return -1;
  }
  ioctl (fd, SNDCTL_DSP_RESET);
  ioctl (fd, SNDCTL_DSP_SPEED, &dsp_speed);
  ioctl (fd, SNDCTL_DSP_STEREO, &stereo);
  ioctl (fd, SNDCTL_DSP_SETFMT, &fmts);
  return fd;
}
#endif
#if SS_SYN_USE_ESD==TRUE
int ss_close(int fd)
{
  return esd_close(fd);
}
#else
int ss_close(int fd)
{
  return close(fd);
}
#endif
int sync_audio(int fd)
{
  return ioctl (fd, SNDCTL_DSP_SYNC);
}

#if SS_SYN_USE_ESD == TRUE
void ss_reset_audio(char *prog_name, char *host)
{
  int sock = -1;
  esd_info_t *einfop;
  esd_player_info_t *eplp;

  if (host == NULL) {
    host = getenv("SPEECH_SERVER_HOST");
  }
  sock = esd_open_sound( host );
  if (sock == -1) {
    printf("can't open control channel\n");
    return;
  }
  einfop = esd_get_all_info(sock);
  eplp = einfop->player_list;
  while(eplp != NULL) {
    printf("%s\n", eplp->name);
    if (strcmp(eplp->name, prog_name) == 0) break;
    eplp = eplp->next;
  }
  if (eplp != NULL) {
    printf("id: %d\n", eplp->source_id);
    fflush(stdout);
    esd_set_stream_pan(sock, eplp->source_id, 0, 0);
    printf("set volume: 0:0\n");
  }
  else {
    printf("set volume failed, stream %d\n", eplp->source_id);
  }
  esd_free_all_info(einfop);
  esd_close(sock);
  return;
}
#else
int ss_reset_audio(int fd)
{
  return ioctl (fd, SNDCTL_DSP_RESET);
}
#endif
/* PCMデータを短縮する */
int  ssWarpPcm(short *data, int len, double factor)
{
  double ratio = ((double)1. / factor);
  int	i, j;
  double	w;

  for (i = 0; i < len; ++i) {
    w = (double) i * ratio;
    j = (int) w;
    w -= (double) j;
    if ((j + 1) >= len)
      {
	break;
      }
    data [i] = /*nint*/ (short)((double)data [j] * (1. - w) 
				+ (double)data [j + 1] * w + 0.5);
  }
  return i;
}

/* skip a `frames' frame of every `unit' frames. */
int ssSkipPcm(short *data, int len, int unit, int frames)
{
  int i;
  int outframes;
  int nframes;
  short *dfrom, *dto;

  if (unit - frames <= 0) {
    /* illegal argument. */
    return len;
  }

  dfrom = dto = data;
  nframes = (len / SS_SKIP_FRAME_SIZE) - unit + frames;
  outframes = 0;
  for (i = 0; i < nframes; i += unit) {
    dfrom = data + i * SS_SKIP_FRAME_SIZE;
    if (dfrom != dto) {
      memcpy((void *)dto, (void *)dfrom,
	     sizeof(short)*SS_SKIP_FRAME_SIZE * (unit - frames));
    }
    outframes += unit - frames;
    dto += SS_SKIP_FRAME_SIZE * (unit  - frames);
  }    
  return outframes * SS_SKIP_FRAME_SIZE;
}
