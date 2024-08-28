/*
** dic.h --- Hash dictionary (header).
**
** Copyright (C) 2000, 2001 Bilingual Emacspeak Project <inoue@argv.org>
**
** Last modified: $Date: 2001/10/12 18:55:16 $ by $Author: inoue $
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
#ifndef __T_DICTIONARY__
#define __T_DICTIONARY__

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#ifndef SEEK_SET
#include <unistd.h>
#endif
#define str_eq(s1,s2) (!strcmp((s1),(s2)))
#define strn_eq(s1,s2, num) (!strncmp((s1),(s2), (num)))
#define HashPrime 499          /*ハッシュテーブルサイズ*/
#define MaxLineLength 128       /*一行の最大文字数*/
#define TAB '\t'                /*コメント開始記号のデフォルト*/
#define SPACE ' '               /*区切り記号のデフォルト*/
#define HeaderBeg "%{\n"        /*ヘッダー開始記号*/
#define HeaderEnd "%}\n"        /*ヘッダー終了記号*/

/*ハッシュ構造体*/
typedef struct _dicRecord {
    char *key;                 /*見出し*/
    char *cont;                /*内容*/
    int attr;         /* integer attribute */
    struct _dicRecord *next;   /*次のdicRecordへのポインタ*/
}dicRecord;

/* 辞書構造体 */
typedef struct _Dictionary {
  dicRecord *dicBase[HashPrime]; /*ハッシュテーブル*/
  char commentSep;               /*コメント開始記号*/
  char key_contSep;              /*区切り記号*/
  char *dicFileName;             /*辞書ファイル名*/
  int eor_flg;                   /*contの有無を示すフラグ*/
  int entryNum;                  /*レコード登録数*/
} Dictionary;

/*プロトタイプ宣言*/

int dicGetKey(Dictionary *, char buf[]);
int dicGetCont(Dictionary *, char buf[],int ofset);
int dicProcessHeader(Dictionary *, FILE *);
unsigned int dicKeyHash(char *);
unsigned int dicNKeyHash(char *, int);
void dicFreeRecords(dicRecord *);
int dicLoadDictionary(Dictionary *, char *);
void dicUnloadDictionary(Dictionary *);
dicRecord *dicHashSearch(Dictionary *, char *);
dicRecord *dicNHashSearch(Dictionary *, char *, int);
char *dicChangeWord(Dictionary *, char *);
char *dicNChangeWord(Dictionary *, char *, int);
char *dicChangeWordX(Dictionary *, char *);
char *dicNChangeWordX(Dictionary *, char *, int);
dicRecord *dicSearchNext(dicRecord *recp, char *key);
dicRecord *dicNSearchNext(dicRecord *recp, char *key,
			  int length);
dicRecord *dicSearchPair(Dictionary *dicp,
			 char *key, char *cont);
dicRecord *dicNSearchPair(Dictionary *dicp, char *key,
			  char *cont, int length1,
			  int length2);
#endif /* !__TANAKA_DICTIONARY__ */
