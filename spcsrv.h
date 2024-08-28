/* -*- mode: c++; tab-width: 2 -*- */
/*
** spcsrv.h --- Various declarations for LinuxSpeechServer
**
** Copyright (C) 2000, 2001 Bilingual Emacspeak Project <inoue@argv.org>
**
** Last modified: $Date: 2001/11/18 14:10:43 $ by $Author: inoue $
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

#ifndef __SPCSRV_H__
#define __SPCSRV_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include "config.h"

/* Function Prototypes */
int ss_close(int fd);
int open_audio(char *audevice);
#if SS_SYN_USE_ESD == TRUE
void ss_make_prog_name(char *namep, int len);
int open_audio(char *host, char *prog_name);
#endif
int sync_audio(int fd);
#if SS_SYN_USE_ESD == TRUE
void ss_reset_audio(char *prog_name, char *host);
#else
int ss_reset_audio(int fd);
#endif
int ssWarpPcm(short *data, int len, double factor);
int ssSkipPcm(short *data, int len, int unit, int frames);
char *ss_read_input(FILE *fp);

#ifdef DEBUG_OUTPUT
extern void log(char *);
#endif
#endif /* __SPCSRV_H__ */

