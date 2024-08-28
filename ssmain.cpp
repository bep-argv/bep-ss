/*
** ssmain.cpp --- Main for LinuxSpeechServer.
**
** Copyright (C) 2000, 2001 Bilingual Emacspeak Project <inoue@argv.org>
**
** Last modified: $Date: 2002/02/12 17:22:48 $ by $Author: inoue $
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
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#define DEBUG_OUTPUT 1
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string>

#include "config.h"
#include "ssdefs.h"
#if defined(SS_OUTLOUD)
#include "outloud.h"
#endif /* SS_OUTLOUD */
#if defined(SS_DTALKER)
#include "dt.h"
#include "dtvoice.h"
#endif /* SS_DTALKER */
#include "spcsrv.h"
#include "cmdproc.h"


int main ( int iArgc, char **ppszArgv )
{
	char *read_buf;
	ssCommandProcessor cmd;
	ssContext sc;
#if defined(SS_DTALKER)
	dTalkParam dt;
	sc.register_tts(&dt, SS_LANG_JA);
#endif /* SS_DTALKER */
#if defined(SS_OUTLOUD)
	TTS_Outloud ol;
	sc.register_tts(&ol, SS_LANG_EN);
#endif /* SS_OUTLOUD */
	if (sc.start() != 0)
	  {
	    printf("Speech server starting faild\n");
	    return 1; // exit from main
	  }
#ifdef WRITE_ELOG
	log_error(NULL, "starting up\n");
#endif

	printf("Bilingual Emacspeak Speech Server Ver. %s started!\n",
	       SS_VERSION);

	while(1) {
		printf ("ready\n");
		read_buf = ss_read_input(stdin);
		if (read_buf == NULL) {
			printf("end of file on input stream\n");
			break;
		}
		if (cmd.dispatch(&sc, read_buf) == 0)
		  break;
	}
	sc.shutdown();
#ifdef WRITE_ELOG
	log_error(NULL, "exited normally\n");
#endif
	return 0;
}

#ifdef WRITE_ELOG
int log_error(FILE * fpLog, char const* fmt, ...)
{
	FILE * fp;
	char FileName[256];
	if (fpLog == NULL) {
		sprintf(FileName, "%s/elog", getenv("HOME"));
		fp = fopen(FileName, "a");
		if (fp == NULL) printf("can't open log\n");
		else printf("opend %s\n", FileName);
	}
	else fp = fpLog;
	va_list ap;
	va_start(ap, fmt);
	int ret = vfprintf(fp, fmt, ap);
	va_end(ap);
	fflush(fp);
	if (fpLog == NULL) {
		fclose(fp);
	}
	return ret;
}
#endif

