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
/* �ǥХå�����/���顼��å������ν��Ϥ����ˤϡ����ιԤ򥳥��Ȥˤ��ޤ��� */
/* #define DEBUG_DIC*/

#include "dic.h"

/* �����Х��ѿ� */
/* for dic(N)ChangeWordX() */
static char DicDummyCont='\0';

/* �ؿ�dicGetKey
	buf������ɤߡ�key_contSep��commentSep��
	�ФƤ�����'\0'�����졢���������ֹ���֤���*/

int dicGetKey(Dictionary *dicp,char buf[])
{
  int i;
  for(i=0;;i++)
    {
      if((buf[i]==dicp->commentSep)||(buf[i]=='\0'))  /*cont���ʤ����*/
	{
	  buf[i]='\0';
	  dicp->eor_flg=1;
	  break;
	}
      else if(buf[i]==dicp->key_contSep)              /*cont��������*/
	{
	  buf[i]='\0';
	  break;
	}
    }
  return(i+1);
}


/*�ؿ�dicGetCont
	buf������ɤߡ�commentSep��'\0'���ФƤ�����
   '��0'�����졢���������ֹ���֤���*/

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


/* �ؿ�dicProcessHeader
	  �إå����μ�갷��
	  �إå�����������������ȳ��ϵ���(comment)��
	  ���ڤ국��(separator)���ѿ�commentSep��key_contSep��
	  ���������إå����ιԿ����֤����إå����Ͼ�ά���ǽ��
	  �إå����ʤ��Ȥ���0���إå����۾�ΤȤ���-1���֤���
	  �إå����Ȥϡ���Ȥ���
		%{
		comment #      �ʥǥե���Ȥϥ��֡�
		separator ,    �ʥǥե���Ȥϥ��ڡ�����
		%}
	  �Ȥ���ȡ�
		key,content#comment
	  �Ȥ�����������ˤʤ롣 */

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


/*�ؿ�dicKeyHash
	���������ơ��ϥå����ͤ��֤���*/

unsigned int dicKeyHash(char *name)
{
  unsigned int hashVal;

  for(hashVal=0;*name;name++)   /*�ϥå����ͤη׻�*/
    hashVal=(*name+31*hashVal)%HashPrime;
  return(hashVal);
}

/*�ؿ�dicNKeyHash
	���������ơ��ϥå����ͤ��֤���Ĺ������С������*/

unsigned int dicNKeyHash(char *name, int length)
{
  unsigned int hashVal=0;
char *endp;

  for(endp=name+length;name<endp;name++)   /*�ϥå����ͤη׻�*/
    hashVal=(*name+31*hashVal)%HashPrime;
  return(hashVal);
}

/* �ؿ� dicFreeRecords()
   �ݥ��󥿤Ǽ����줿dicRecord��¤�Τȡ������Ϣ�ʤ�ꥹ�Ȥ�Ƶ�Ū�˺��*/
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

/*�ؿ�dicLoadDictionary
	key��cont��ϥå��幽¤������֤���
	����ȥե�����̾������Ȥ��롣*/
/* ����: 0=���ｪλ 1=����ط����顼 2=�ե����륪���ץ󥨥顼 
   3=�إå������ΰ۾� */

int dicLoadDictionary(Dictionary *dicp,char *fileName)
{
  int sep, sep2, i,hashNum, llen;
  char buf[MaxLineLength];
  dicRecord *tmp;
  FILE *fp;

  sep = sep2 = 0;

  for(i=0;i<HashPrime;i++) /*�ϥå���ơ��֥�ν����*/
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
      if(llen>0) buf[--llen]='\0'; /* �Ǹ�����פ�'\n'���� */
      if(llen>0 && buf[llen-1] == '\r')
	buf[--llen] = '\0';	/* ���פ�CR������к�� */
      if(llen==0 || buf[0]==dicp->commentSep || buf[0]==dicp->key_contSep)
	continue; /* ��̣�Τʤ��Ԥʤ�̵�� */
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
      tmp->key=NULL; /* ����� */
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

/* �ؿ�dicUnloadDictionary()
   ����Τ���˳��ݤ��������������롣*/
void dicUnloadDictionary(Dictionary *dicp)
{
  int i;
  /* �ϥå���ơ��֥��ʬ�����֤��� */
  for (i = 0; i < HashPrime; i++) {
    dicFreeRecords(dicp->dicBase[i]);
    dicp->dicBase[i] = NULL;
  }
  free(dicp->dicFileName);
  dicp->dicFileName = NULL;
  return;
}

/*�ؿ�dicHashSearch
	key�����ơ�����򸡺����롣
	¸�ߤ����餽��key��õ���ݥ��󥿤��֤���
	¸�ߤ��ʤ��ä���NULL���֤���*/

dicRecord *dicHashSearch(Dictionary *dicp,char *key)
{
  int hashNum;             /*�ϥå�����*/
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

/* �ؿ�dicNHashSearch: dicHashSearch��Ĺ������С������
   �裳������key����ȹ�����Ѥ���Ĺ������� */

dicRecord *dicNHashSearch(Dictionary *dicp,char *key, int length)
{
  int hashNum;             /*�ϥå�����*/
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

/*�ؿ�dicChangeWord
	key�����ơ���������key��cont���֤���
	�ҥåȤ����쥳���ɤ�cont�����ʤ�NULL���֤��� */

char *dicChangeWord(Dictionary *dicp,char *key)
{
  dicRecord *tmp;

  tmp=dicHashSearch(dicp,key);
  if(tmp==NULL || tmp->cont==NULL)
    return(NULL);
  return(tmp->cont);
}

/* �ؿ�dicNChangeWord: dicChangeWord Ĺ������С������ */

char *dicNChangeWord(Dictionary *dicp,char *key,int length)
{
  dicRecord *tmp;

  tmp=dicNHashSearch(dicp,key,length);
  if(tmp==NULL || tmp->cont==NULL)
    return(NULL);
  return(tmp->cont);
}

/*�ؿ�dicChangeWordX
	key�����ơ���������key��cont���֤���NULL���֤������
	���顼��å�������Ф���""�����ä����ؤΥݥ��󥿤��֤���*/

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

/* �ؿ�dicSearchNext dicHashSearch�Ǹ��Ĥ��ä��쥳���ɤΥݥ��󥿤ȥ���
   �������ꡢƱ��Υ�������ĥ쥳���ɤ�ꥹ�����õ�����롣 */
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

/* �ؿ�dicNSearchNext: dicSearchNext Ĺ������С������ */

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

/* �ؿ�dicSearchPair
   ���������ƤΥڥ��������ꡢ�������õ������Ʊ���ڥ�������Ф���
   dicRecord�Υݥ��󥿤򡢤ʤ����NULL���֤��� */
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

/* �ؿ�dicNSearchPair
   ���������ƤΥڥ��������ꡢ�������õ������Ʊ���ڥ�������Ф���
   dicRecord�Υݥ��󥿤򡢤ʤ����NULL���֤���
 ���������ƤΤ��줾���Ĺ������ꤹ��С������ */
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

