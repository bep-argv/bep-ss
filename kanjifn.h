/*
** kanjifn.h --- Kanji-related functions(header).
**
** Copyright (C) 2000, 2001 Bilingual Emacspeak Project <inoue@argv.org>
**
** Last modified: $Date: 2001/10/14 15:31:30 $ by $Author: inoue $
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
#ifndef __KANJIFN_H
#define __KANJIFN_H
#include <stdio.h>
#define iskanji(c) \
((((c) & 0xff)>=0x81 && ((c) & 0xff ) <= 0x9f || ((c) & 0xff)>= 0xe0 && ((c) & 0xff)<=0xfc) ? 1 : 0)
#define iskanji2(c) \
((((c) >= 0x40 && (c) <= 0x7e ) || ((c) >=0x80 && (c)<= 0xfc )) ? 1 : 0)
#define iszenalpha(c) \
(((*c) == 0x81 && (*(c+1)) == 0x66) \
|| ((*c) == 0x82 && \
(((*(c+1)) >= 0x60 && (*(c+1)) <= 0x79)\
|| ((*(c+1)) >= 0x81 && (*(c+1)) <= 0x9a))))

#define iskatakana(c) \
((((*(c) & 0xff) == 0x81 ) && ((*(c+1) & 0xff) == 0x5b )) \
|| (((*(c) & 0xff) == 0x83 ) \
&& (((*(c+1) & 0xff) >= 0x40 ) || ((*(c+1) & 0xff) <= 0x96 ))) \
|| ((*(c) & 0xff) >= 0xa0 && (*(c) & 0xff) <= 0xdf))

/* constant */
#define ENG_KANA_MAX_FLAGMENT 10
#define ROMA_STR_TIMES 4

struct roma_tbl_t {
  char word[5];
  int  str_length;
};
						 
#define ROMA_ACCEPT last = i;
#define ROMA_FAIL free(buf); return FALSE
#define ROMA_EXIST_NEXT (i + 1 < inLen)
#define ROMA_NEXT_CHAR i++;

#define S_A    0
#define S_K    1
#define S_KY   2
#define S_S    3
#define S_SY   4
#define S_SH   5
#define S_T    6
#define S_TY   7
#define S_CH   8
#define S_N    9
#define S_NY  10
#define S_H   11
#define S_HY  12
#define S_F   13
#define S_M   14
#define S_MY  15
#define S_R   16
#define S_RY  17
#define S_G   18
#define S_GY  19
#define S_Z   20
#define S_ZY  21
#define S_J   22
#define S_JY  23
#define S_D   24
#define S_DY  25
#define S_B   26
#define S_BY  27
#define S_V   28
#define S_P   29
#define S_PY  30

/* prototypes */
char jpZenToHan(char *);

/* convert alphanumeric string to Katakana reading string */
int jpEngToKana(Dictionary *dicp, char *pszIn, int inLen, char **ppszOut);

/* convert Katakana reading string to Roma string */
int jpRomaToKana(char *pszIn, int inLen, char **ppszOut);

/* convert Katakana reading char to Roma string */
int jpRomaToKana_vowel(char pszIn, int i, char **p);

#endif
