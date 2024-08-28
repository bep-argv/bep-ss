/*
** cmdproc.h --- subroutines respect to each command.
**
** Copyright (C) 2000, 2001 Bilingual Emacspeak Project <inoue@argv.org>
**
** Last modified: $Date: 2002/02/22 17:09:16 $ by $Author: inoue $
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
#ifndef CMDPROC_H
#define CMDPROC_H

#include "context.h"

class ssCommandProcessor;
struct ssCommandMap {
  char *name;
  int (ssCommandProcessor::*func)(ssContext *, char *, char **);
};

class ssCommandProcessor {
  int q (ssContext *, char *, char **);	/* 0 */
  int d (ssContext *, char *, char **);	/* 1 */
  int s (ssContext *, char *, char **);	/* 2 */
  int tts_set_speech_rate(ssContext *, char *, char **); /* 3 */
  int tts_set_punctuations(ssContext *, char *, char **); /* 4 */
  int tts_say(ssContext *, char *, char **); /* 5 */
  int l (ssContext *, char *, char **);	/* 6 */
  int t (ssContext *, char *, char **);	/* 7 */
  int tts_set_language(ssContext *, char *, char **); /* 8 */
  int tts_set_character_scale(ssContext *, char *, char **); /* 9 */
  int tts_sync_state(ssContext *, char *, char **); /* 10 */
  int tts_allcaps_beep(ssContext *, char *, char **); /* 11 */
  int tts_capitalize(ssContext *, char *, char **); /* 12 */
  int tts_split_caps(ssContext *, char *, char **); /* 13 */
  int tts_set_rate_offset(ssContext *, char *, char **); /* 14 */

#define TOTAL_COMMAND_NUM 15
  string cname[TOTAL_COMMAND_NUM];
  int (ssCommandProcessor::*cfunc[TOTAL_COMMAND_NUM])
    (ssContext *, char *, char **);
 public:
  ssCommandProcessor();
  int dispatch(ssContext *sc, char *cmd_buf); 
};

#endif
