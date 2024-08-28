/*
** dic.cpp --- Hash dictionary
**
** Copyright (C) 2000, 2001 Bilingual Emacspeak Project <inoue@argv.org>
**
** Last modified: $Date: 2002/05/05 16:44:16 $ by $Author: inoue $
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
/* デバッグ情報/エラーメッセージの出力をやめるには、下の行をコメントにします。 */
/* #define DEBUG_DIC*/

#include "dic.h"

/* グローバル変数 */
/* for dic(N)ChangeWordX() */
static char DicDummyCont='\0';

/* 関数dicGetKey
	bufの中を読み、key_contSepかcommentSepが
	出てきたら'\0'を入れ、次の配列番号を返す。*/

int dicGetKey(Dictionary *dicp,char buf[])
{
  int i;
  for(i=0;;i++)
    {
      if((buf[i]==dicp->commentSep)||(buf[i]=='\0'))  /*contがない場合*/
	{
	  buf[i]='\0';
	  dicp->eor_flg=1;
	  break;
	}
      else if(buf[i]==dicp->key_contSep)              /*contがある場合*/
	{
	  buf[i]='\0';
	  break;
	}
    }
  return(i+1);
}


/*関数dicGetCont
	bufの中を読み、commentSepか'\0'が出てきたら
   '￥0'を入れ、次の配列番号を返す。*/

int dicGetCont(Dictionary *dicp,char buf[],int ofset)
{
  int i;
  for(i=ofset;;i++)
    {
      if((buf[i]==dicp->commentSep)||(buf[i]=='\0'))
	{
	  buf[i]='\0';
	  dicp->eor_flg=1;
	  break;
	}
    }
  return(i+1);
}


/* 関数dicProcessHeader
	  ヘッダーの取り扱い
	  ヘッダーで定義したコメント開始記号(comment)と
	  区切り記号(separator)を変数commentSepとkey_contSepに
	  代入し、ヘッダーの行数を返す。ヘッダーは省略も可能。
	  ヘッダがないときは0、ヘッダが異常のときは-1を返す。
	  ヘッダーとは、例として
		%{
		comment #      （デフォルトはタブ）
		separator ,    （デフォルトはスペース）
		%}
	  とすると、
		key,content#comment
	  という辞書形式になる。 */

int dicProcessHeader(Dictionary *dicp,FILE *pfp)
{
  int sep=0,lineCnt=0;
  char buf[MaxLineLength];

  buf[0]='\0';
  while(buf[0]=='\0')
    {
      fgets(buf,MaxLineLength,pfp);
      dicGetKey(dicp, buf);
    }
  if(!str_eq(buf,HeaderBeg))
    {
      fseek(pfp, 0, SEEK_SET);
#ifdef DEBUG_DIC
      fprintf(stderr, "dictionary %s header processing is not required\n", dicp->dicFileName);
#endif
      return(0);
    }
  if(!str_eq(buf,HeaderEnd))
    while(1)
      {
	fgets(buf,MaxLineLength,pfp);
	lineCnt++;
	sep=dicGetKey(dicp, buf);
	if(str_eq(buf,"comment"))
	  dicp->commentSep=buf[sep];
	else if(str_eq(buf,"separator"))
	  dicp->key_contSep=buf[sep];
	else if(str_eq(buf,HeaderEnd))
	  {
	    lineCnt--;
	    break;
	  }
	else
	  {
#ifdef DEBUG_DIC
	    fprintf(stderr, "%s: invalid header\n", dicp->dicFileName);
#endif
	    return -1;
	  }
      }
  return(lineCnt);
}


/*関数dicKeyHash
	キーを得て、ハッシュ値を返す。*/

unsigned int dicKeyHash(char *name)
{
  unsigned int hashVal;

  for(hashVal=0;*name;name++)   /*ハッシュ値の計算*/
    hashVal=(*name+31*hashVal)%HashPrime;
  return(hashVal);
}

/*関数dicNKeyHash
	キーを得て、ハッシュ値を返す。長さ指定バージョン*/

unsigned int dicNKeyHash(char *name, int length)
{
  unsigned int hashVal=0;
char *endp;

  for(endp=name+length;name<endp;name++)   /*ハッシュ値の計算*/
    hashVal=(*name+31*hashVal)%HashPrime;
  return(hashVal);
}

/* 関数 dicFreeRecords()
   ポインタで示されたdicRecord構造体と、それに連なるリストを再帰的に削除*/
void dicFreeRecords(dicRecord *recp)
{
  if (recp == NULL)
    return;
  free(recp->key);
  free(recp->cont);
  dicFreeRecords(recp->next);
  free(recp);
  return;
}

/*関数dicLoadDictionary
	keyとcontをハッシュ構造の中に置く。
	辞書とファイル名を引数とする。*/
/* 返値: 0=正常終了 1=メモリ関係エラー 2=ファイルオープンエラー 
   3=ヘッダ形式の異常 */

int dicLoadDictionary(Dictionary *dicp,char *fileName)
{
  int sep, sep2, i,hashNum, llen;
  char buf[MaxLineLength];
  dicRecord *tmp;
  FILE *fp;

  sep = sep2 = 0;

  for(i=0;i<HashPrime;i++) /*ハッシュテーブルの初期化*/
    dicp->dicBase[i]=NULL;
  dicp->commentSep=TAB;
  dicp->key_contSep=SPACE;
  dicp->eor_flg=0;
  dicp->entryNum=0;
  dicp->dicFileName=NULL;
  if((dicp->dicFileName=(char *)malloc(sizeof(char)*strlen(fileName)+1))
     ==NULL)
    {
#ifdef DEBUG_DIC
      fprintf(stderr, "memory allocation error\n");
#endif
      return 1;
    }
  strcpy(dicp->dicFileName,fileName);
#ifdef MSDOS
  if((fp=fopen(dicp->dicFileName,"rt"))==NULL)
#else
    if((fp=fopen(dicp->dicFileName,"r"))==NULL)
#endif
      {
#ifdef DEBUG_DIC
	fprintf(stderr, "file %s can not be opend\n",dicp->dicFileName);
#endif
	return 2;
      }
  if(dicProcessHeader(dicp, fp)<0) return 3;
  while(fgets(buf,MaxLineLength,fp)!=NULL)
    {
      llen=strlen(buf);
      if(llen>0) buf[--llen]='\0'; /* 最後の不要な'\n'を削除 */
      if(llen>0 && buf[llen-1] == '\r')
	buf[--llen] = '\0';	/* 不要なCRがあれば削除 */
      if(llen==0 || buf[0]==dicp->commentSep || buf[0]==dicp->key_contSep)
	continue; /* 意味のない行なら無視 */
      dicp->eor_flg=0;
      sep=dicGetKey(dicp, buf);
      hashNum=dicKeyHash(buf);
      if((tmp=(dicRecord *)malloc(sizeof(dicRecord)))==NULL)
	{
#ifdef DEBUG_DIC
	  fprintf(stderr, "%s: object generation error\n", dicp->dicFileName);
#endif
	  return 1;
	}
      tmp->key=NULL; /* 初期化 */
      tmp->cont=NULL;
      tmp->attr = 0;
      tmp->next=NULL;
      if((tmp->key=(char *)malloc(sizeof(char)*(strlen(buf)+1)))
	 ==NULL)
	{
#ifdef DEBUG_DIC
	  fprintf(stderr, "memory allocation error\n");
#endif
	  return 1;
	}
      strcpy(tmp->key,buf);
      if(!(dicp->eor_flg))
	{
	  sep2 = dicGetKey(dicp,&buf[sep]);
	  if((tmp->cont=(char *)malloc(sizeof(char)*(strlen
						     (&buf[sep])+1)))==NULL)
	    {
#ifdef DEBUG_DIC
	      fprintf(stderr, "memory allocation error\n");
#endif	      
	      return 1;
	    }
	  strcpy(tmp->cont,&buf[sep]);
	}
      /* read attribute */
      if (!(dicp->eor_flg)) {
	dicGetCont(dicp, buf, sep + sep2);
	tmp->attr = atoi(&buf[sep + sep2]);
      }
      tmp->next=dicp->dicBase[hashNum];
      dicp->dicBase[hashNum]=tmp;
      dicp->entryNum++;
    }
  fclose(fp);
  return 0;
}

/* 関数dicUnloadDictionary()
   辞書のために確保したメモリを解放する。*/
void dicUnloadDictionary(Dictionary *dicp)
{
  int i;
  /* ハッシュテーブルの分繰り返す。 */
  for (i = 0; i < HashPrime; i++) {
    dicFreeRecords(dicp->dicBase[i]);
    dicp->dicBase[i] = NULL;
  }
  free(dicp->dicFileName);
  dicp->dicFileName = NULL;
  return;
}

/*関数dicHashSearch
	keyを得て、辞書を検索する。
	存在したらそのkeyを探すポインタを返す。
	存在しなかったらNULLを返す。*/

dicRecord *dicHashSearch(Dictionary *dicp,char *key)
{
  int hashNum;             /*ハッシュ値*/
  dicRecord *tmp;

  hashNum=dicKeyHash(key);
  if(dicp->dicBase[hashNum]==NULL)
    return(NULL);
  else if(str_eq(dicp->dicBase[hashNum]->key,key))
    return(dicp->dicBase[hashNum]);
  else
    {
      tmp=dicp->dicBase[hashNum];
      while(tmp!=NULL)
	{
	  if(str_eq(tmp->key,key))
	    break;
	  tmp=tmp->next;
	}
      return(tmp);
    }
}

/* 関数dicNHashSearch: dicHashSearchの長さ指定バージョン
   第３引数にkeyから照合に利用する長さを指定 */

dicRecord *dicNHashSearch(Dictionary *dicp,char *key, int length)
{
  int hashNum;             /*ハッシュ値*/
  dicRecord *tmp;

  hashNum=dicNKeyHash(key, length);
  if(dicp->dicBase[hashNum]==NULL)
    return(NULL);
  else if(strn_eq(dicp->dicBase[hashNum]->key,key,length)
	  && (int)strlen(dicp->dicBase[hashNum]->key) == length)
    return(dicp->dicBase[hashNum]);
  else
    {
      tmp=dicp->dicBase[hashNum];
      while(tmp!=NULL)
	{
	  if(strn_eq(tmp->key,key,length)
	     && (int)strlen(tmp->key) == length)
	    break;
	  tmp=tmp->next;
	}
      return(tmp);
    }
}

/*関数dicChangeWord
	keyを得て、検索したkeyのcontを返す。
	ヒットしたレコードのcontが空ならNULLを返す。 */

char *dicChangeWord(Dictionary *dicp,char *key)
{
  dicRecord *tmp;

  tmp=dicHashSearch(dicp,key);
  if(tmp==NULL || tmp->cont==NULL)
    return(NULL);
  return(tmp->cont);
}

/* 関数dicNChangeWord: dicChangeWord 長さ指定バージョン */

char *dicNChangeWord(Dictionary *dicp,char *key,int length)
{
  dicRecord *tmp;

  tmp=dicNHashSearch(dicp,key,length);
  if(tmp==NULL || tmp->cont==NULL)
    return(NULL);
  return(tmp->cont);
}

/*関数dicChangeWordX
	keyを得て、検索したkeyのcontを返す。NULLを返す代わりに
	エラーメッセージを出し、""の入った場所へのポインタを返す。*/

char *dicChangeWordX(Dictionary *dicp,char *key)
{
  dicRecord *tmp;

  tmp=dicHashSearch(dicp,key);
  if(tmp==NULL || tmp->cont==NULL) {
#ifdef DEBUG_DIC
    fprintf(stderr, "\ndicChangeWordX: cont not found: %s\n", key);
#endif
    return (&DicDummyCont);
  }
  return(tmp->cont);
}

char *dicNChangeWordX(Dictionary *dicp,char *key,int length)
{
  dicRecord *tmp;

  tmp=dicNHashSearch(dicp,key,length);
  if(tmp==NULL || tmp->cont == NULL) {
#ifdef DEBUG_DIC
    int i;
    fprintf(stderr, "\ndicNChangeWordX: cont not found: ");
    for(i=0; i<length; i++)
      fputc(key[i], stderr);
    fputc('\n', stderr);
#endif
    return(&DicDummyCont);
  }
  return(tmp->cont);
}

/* 関数dicSearchNext dicHashSearchで見つかったレコードのポインタとキー
   を受け取り、同一のキーを持つレコードをリスト内で探索する。 */
dicRecord *dicSearchNext(dicRecord *recp, char *key)
{
  dicRecord *tmp;
  if(recp==NULL) return(NULL);
  tmp=recp->next;
  while(tmp!=NULL) {
    if(str_eq(tmp->key, key)) break;
    tmp=tmp->next;
  }
  return(tmp);
}

/* 関数dicNSearchNext: dicSearchNext 長さ指定バージョン */

dicRecord *dicNSearchNext(dicRecord *recp, char *key, int length)
{
  dicRecord *tmp;
  if(recp==NULL) return(NULL);
  tmp=recp->next;
  while(tmp!=NULL) {
    if(strn_eq(tmp->key, key, length)) break;
    tmp=tmp->next;
  }
  return(tmp);
}

/* 関数dicSearchPair
   キーと内容のペアを受け取り、辞書内を探索して同じペアがあればその
   dicRecordのポインタを、なければNULLを返す。 */
dicRecord *dicSearchPair(Dictionary *dicp, char *key, char *cont)
{
  dicRecord *tmp;
  tmp=dicHashSearch(dicp, key);
  while(tmp!=NULL)
    {
      if((tmp->cont!=NULL) && str_eq(cont, tmp->cont)) break;
      else tmp=dicSearchNext(tmp, key);
    }
  return(tmp);
}

/* 関数dicNSearchPair
   キーと内容のペアを受け取り、辞書内を探索して同じペアがあればその
   dicRecordのポインタを、なければNULLを返す。
 キーと内容のそれぞれに長さを指定するバージョン */
dicRecord *dicNSearchPair(Dictionary *dicp, char *key, char *cont,
			  int length1, int length2)
{
  dicRecord *tmp;
  tmp=dicNHashSearch(dicp, key, length1);
  while(tmp!=NULL)
    {
      if((tmp->cont!=NULL) && strn_eq(cont, tmp->cont, length2)) break;
      else tmp=dicNSearchNext(tmp, key, length1);
    }
  return(tmp);
}

