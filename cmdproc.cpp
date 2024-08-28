/*
** cmdproc.cpp --- subroutines respect to each command.
**
** Copyright (C) 2000, 2001 Bilingual Emacspeak Project <inoue@argv.org>
**
** Last modified: $Date: 2002/02/22 17:10:07 $ by $Author: inoue $
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
#include <string>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "ssdefs.h"
#include "spcsrv.h"
#include "dic.h"
#include "kanjifn.h"
#include "context.h"
#include "cmdproc.h"

ssCommandProcessor::ssCommandProcessor()
{
  int i = 0;
  cname[i] = "q";
  cfunc[i++] = &ssCommandProcessor::q;
  cname[i] = "d";
  cfunc[i++] = &ssCommandProcessor::d;
  cname[i] = "s";
  cfunc[i++] = &ssCommandProcessor::s;
  cname[i] = "tts_set_speech_rate";
  cfunc[i++] = &ssCommandProcessor::tts_set_speech_rate;
  cname[i] = "tts_set_punctuations";
  cfunc[i++] = &ssCommandProcessor::tts_set_punctuations;
  cname[i] = "tts_say";
  cfunc[i++] = &ssCommandProcessor::tts_say;
  cname[i] = "l";
  cfunc[i++] = &ssCommandProcessor::l;
  cname[i] = "t";
  cfunc[i++] = &ssCommandProcessor::t;
  cname[i] = "tts_set_language";
  cfunc[i++] = &ssCommandProcessor::tts_set_language;
  cname[i] = "tts_set_character_scale";
  cfunc[i++] = &ssCommandProcessor::tts_set_character_scale;
  cname[i] = "tts_sync_state";
  cfunc[i++] = &ssCommandProcessor::tts_sync_state;
  cname[i] = "tts_allcaps_beep";
  cfunc[i++] = &ssCommandProcessor::tts_allcaps_beep;
  cname[i] = "tts_capitalize";
  cfunc[i++] = &ssCommandProcessor::tts_capitalize;
  cname[i] = "tts_split_caps";
  cfunc[i++] = &ssCommandProcessor::tts_split_caps;
  cname[i] = "tts_set_rate_offset";
  cfunc[i++] = &ssCommandProcessor::tts_set_rate_offset;
}

int
ssCommandProcessor::dispatch(ssContext *sc, char *cmd_buf)
{
  int cmdIdx;
  int iRet;
  char *nextCmd;
  cout << cmd_buf << endl;
#ifdef WRITE_ELOG
  log_error(NULL, "Input: \"%s\"\n", cmd_buf);
#endif
  nextCmd = cmd_buf;
  while (*nextCmd == ' ') nextCmd++;
  if(*nextCmd == '\0') {
    cout << "null command\n" << endl;
#ifdef WRITE_ELOG
    log_error(NULL, "null command");
#endif
    return 255;
  }
  // skip command
  while (*nextCmd != '\0' && *nextCmd != ' ') nextCmd++;

  /* Search correspoinding command function */
  for (cmdIdx = 0; cmdIdx < TOTAL_COMMAND_NUM; cmdIdx++){
    if (cname[cmdIdx].compare(cmd_buf, 0, (nextCmd - cmd_buf)) == 0)
      break;
  }
  if (cmdIdx < TOTAL_COMMAND_NUM) {
    cout << "cmd: " << cname[cmdIdx] << endl;
    // call function
    iRet = (this->*cfunc[cmdIdx])(sc, cmd_buf, &nextCmd);
    if (cfunc[cmdIdx] == &ssCommandProcessor::q
	&& iRet != 0) {
      while (iRet != 0) {
	cout << "queue continuing ready\n" << endl;
	cmd_buf = ss_read_input(stdin);
	if (cmd_buf == NULL) {
	  cmd_buf = "";
	}
	cout << cmd_buf << endl;
#ifdef WRITE_ELOG
	log_error(NULL, "Input+: \"%s\"\n", cmd_buf);
#endif
	nextCmd = cmd_buf;
	iRet = q(sc, nextCmd, &nextCmd);
      }
    }
  } else {
    cout << "ignoring: " << cmd_buf << endl;
  }
  return 1;
}

int
ssCommandProcessor::q(ssContext *sc, char *pcCmd, char **pcEndCmd)
{
  char *p = pcCmd;
  char *str_begin;
  int iRet;
  int adding;

  if (*p != 'q')
    adding = 1;
  else {
    adding = 0;
    while (*p != '{' && *p != '\0')
      p++;
  }
  if (*p == '\0') {
    *pcEndCmd = p;
    return 1;
  }

  if (*p == '{') p++;
  str_begin = p;

  while (*p != '}' && *p != '\0') {
    if (iskanji(*p)) p+=2;
    else p++;
  }
  if (adding) {
    sc->push_text_add(str_begin, (p - str_begin));
  } else {
    sc->push_text(str_begin, (p - str_begin), SS_WORK_TYPE_PLAIN);
  }
  if (*p == '}') {
    *pcEndCmd = p+1;
    iRet = 0;
  } else {
    *pcEndCmd = p;
    iRet = 1;
  }
  return iRet;
}

int
ssCommandProcessor::d(ssContext *sc, char *pcCmd, char **pcEndCmd)
{
  if (*pcCmd == '\0') {
    *pcEndCmd = pcCmd;
    return 0;
  }

  sc->speak();

  *pcEndCmd = pcCmd+1;
  return 0;
}

int
ssCommandProcessor::s(ssContext *sc, char *pcCmd, char **pcEndCmd)
{
  if (*pcCmd == '\0') {
    *pcEndCmd = pcCmd;
    return 0;
  }
  cout << "stopping\n" << endl;
  sc->stop(1);

  *pcEndCmd = pcCmd+1;
  return 0;
}

int
ssCommandProcessor::tts_set_speech_rate(ssContext *sc, char *pcCmd, char **pcEndCmd)
{
  int speed;
  if (*pcCmd == '\0') {
    *pcEndCmd = pcCmd;
    return 0;
  }

  while(*pcCmd != ' ' && *pcCmd != '\0')
    pcCmd++;
  if (*pcCmd == '\0') {
    *pcEndCmd = pcCmd;
    return 0;
  }
  while (*pcCmd == ' ')
    pcCmd++;
  speed = atoi(pcCmd);
  sc->set_speed(speed);
  while (*pcCmd >= '0' && *pcCmd <= '9')
    pcCmd++;
  *pcEndCmd = pcCmd;
  return 0;
}

int
ssCommandProcessor::tts_set_punctuations(ssContext *sc, char *pcCmd, char **pcEndCmd)
{
  if (*pcCmd == '\0') {
    *pcEndCmd = pcCmd;
    return 0;
  }

  while(*pcCmd != ' ' && *pcCmd != '\0')
    pcCmd++;
  if (*pcCmd == '\0') {
    *pcEndCmd = pcCmd;
    return 0;
  }
  while (*pcCmd == ' ')
    pcCmd++;
  if (strncmp("all", pcCmd, 3)==0)
     sc->set_punctuation(SS_PUNCTUATION_ALL);
  if (strncmp("some", pcCmd, 4)==0)
    sc->set_punctuation(SS_PUNCTUATION_SOME);
  if (strncmp("none", pcCmd, 4) == 0)
    sc->set_punctuation(SS_PUNCTUATION_NONE);
  while (*pcCmd >= '0' && *pcCmd <= '9')
    pcCmd++;
  *pcEndCmd = pcCmd;
  return 0;
}

int
ssCommandProcessor::tts_say(ssContext *sc, char *pcCmd, char **pcEndCmd)
{
  char *p = pcCmd;
  char *str_begin;
  int iRet;

  while (*p != '{' && *p != '\0')
    p++;
  if (*p == '\0') {
    *pcEndCmd = p;
    return 0;
  }

  if (*p == '{') p++;
  str_begin = p;

  int iteration = 0;
  while (*p != '}' && *p != '\0') {
    while (*p != '}' && *p != '\0') {
      if (iskanji(*p)) p+=2;
      else p++;
    }
    // if the string is not empty
    if (p != str_begin) {
      if (iteration) {
	sc->push_text_add(str_begin, (p - str_begin));
      } else {
	sc->push_text(str_begin, (p - str_begin), SS_WORK_TYPE_PLAIN);
      }
    }
    str_begin = p;
    iteration++;
  }
  if (*p == '}') {
    *pcEndCmd = p+1;
    iRet = 0;
  } else {
    *pcEndCmd = p;
    iRet = 1;
  }
  sc->speak();
  return iRet;
}

int
ssCommandProcessor::l(ssContext *sc, char *pcCmd, char **pcEndCmd)
{
  char *p = pcCmd;
  char *str_begin;
  string text = "";

  while (*p != '{' && *p != '\0')
    p++;
  if (*p == '\0') {
    *pcEndCmd = p;
    return 0;
  }

  if (*p == '{') p++;
  str_begin = p;

  int iteration = 0;
  while (*p != '}' && *p != '\0') {
    while (*p != '}' && *p != '\0') {
      if (iskanji(*p)) p+=2;
      else p++;
    }
    // if the string is not empty
    if (p != str_begin) {
      if (iteration) {
	text.append(str_begin, (p - str_begin));
      } else {
	text.assign(str_begin, (p - str_begin));
      }
    }
    str_begin = p;
    iteration++;
  }
  sc->push_text((char *)text.c_str(), text.length(), SS_WORK_TYPE_LETTER);
  sc->speak();
  if (*p == '}') {
    *pcEndCmd = p+1;
  } else {
    *pcEndCmd = p;
  }
  return 0;
}

int
ssCommandProcessor::t(ssContext *sc, char *pcCmd, char **pcEndCmd)
{
  char *p = pcCmd;
  char *arg_begin;

  while (!isdigit(*p) && *p != '\0')
    p++;
  if (*p == '\0') {
    *pcEndCmd = p;
    return 0;
  }
  arg_begin = p;
  while(*p != '\0') p++;

  cout << "queueing tone: [" << arg_begin << "]" << endl;
  sc->push_text(arg_begin, p - arg_begin, SS_WORK_TYPE_TONE);
  *pcEndCmd = p;
  return 0;
}

int
ssCommandProcessor::tts_set_language(ssContext *sc, char *pcCmd, char **pcEndCmd)
{
  char *p = pcCmd;
  char lname[SS_MAX_LANGNAME];
  int i = 0;
  while(*p != ' ')
    p++;
  while(!isalpha(*p))
    p++;
  while(i < SS_MAX_LANGNAME && isalpha(*p)) {
    lname[i] = *p++;
    i++;
  }
  lname[i] = '\0';
  sc->set_language(lname);
  *pcEndCmd = p;
  return 0;
}

int
ssCommandProcessor::tts_set_character_scale(ssContext *sc, char *pcCmd, char **pcEndCmd)
{
  char *p = pcCmd;
  char *endptr;
  double scale;
  while(*p != ' ')
    p++;
  scale = strtod(p, &endptr);
  if (endptr != p) {
    sc->set_char_scale((float)scale);
#ifdef WRITE_ELOG
    log_error(NULL, "cmdproc: new char_scale: %f\n", scale);
#endif
  } else {
#ifdef WRITE_ELOG
    log_error(NULL, "cmdproc: Can't set char_scale to %s\n", p);
#endif
  }
  *pcEndCmd = p;
  return 0;
}

int
ssCommandProcessor::tts_sync_state(ssContext *sc, char *pcCmd, char **pcEndCmd)
{
  char punct[6];
  int cap, beep, split, speed;
  while(*pcCmd != '\0' && *pcCmd != ' ')
    pcCmd++;
  int iRet = sscanf(pcCmd, " %5s %d %d %d %d",
		    punct, &cap, &beep, &split, &speed);
  if (iRet != 5) {
    cout << "cmd: invalid sync_state args" << endl;
    return 0;
  }
  if (strncmp("all", punct, 3)==0)
      sc->set_punctuation(SS_PUNCTUATION_ALL);
    else if (strncmp("some", punct, 4)==0)
      sc->set_punctuation(SS_PUNCTUATION_SOME);
    else if (strncmp("none", punct, 4) == 0)
      sc->set_punctuation(SS_PUNCTUATION_NONE);
  sc->set_capitalize(cap);
  sc->set_allcaps_beep(beep);
  sc->set_split_caps(split);
  sc->set_speed(speed);
  return 0;
}

int
ssCommandProcessor::tts_capitalize(ssContext *sc, char *pcCmd, char **pcEndCmd)
{
  char *p = pcCmd;
  char *endptr;
  int cap;
  while(*p != ' ')
    p++;
  cap = strtol(p, &endptr, 10);
  sc->set_capitalize(cap);
#ifdef WRITE_ELOG
    log_error(NULL, "cmdproc: new capitalization: %d\n", cap);
#endif
  *pcEndCmd = p;
  return 0;
}

int
ssCommandProcessor::tts_allcaps_beep(ssContext *sc, char *pcCmd, char **pcEndCmd)
{
  char *p = pcCmd;
  char *endptr;
  int cap;
  while(*p != ' ')
    p++;
  cap = strtol(p, &endptr, 10);
  sc->set_allcaps_beep(cap);
#ifdef WRITE_ELOG
    log_error(NULL, "cmdproc: new allcaps_beep: %d\n", cap);
#endif
  *pcEndCmd = p;
  return 0;
}

int
ssCommandProcessor::tts_split_caps(ssContext *sc, char *pcCmd, char **pcEndCmd)
{
  char *p = pcCmd;
  char *endptr;
  int cap;
  while(*p != ' ')
    p++;
  cap = strtol(p, &endptr, 10);
  sc->set_split_caps(cap);
#ifdef WRITE_ELOG
    log_error(NULL, "cmdproc: new split_caps: %d\n", cap);
#endif
  *pcEndCmd = p;
  return 0;
}

int
ssCommandProcessor::tts_set_rate_offset(ssContext *sc, char *pcCmd, char **pcEndCmd)
{
  char lang[10];
  int offset;
  while (*pcCmd != '\0' && *pcCmd != ' ')
    pcCmd++;
  int iRet = sscanf(pcCmd, " %10s %d", lang, &offset);
  if (iRet != 2) {
    cout << "cmd: invalid rate offset" << endl;    
#ifdef WRITE_ELOG
    log_error(NULL, "cmd: invalid offset spec: %s", pcCmd);
#endif
    return 0;
  }
  // set offset
  sc->setRateOffset(lang, offset);
  return offset;
}
