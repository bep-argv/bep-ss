/*
** kanjifn.cpp --- Kanji-related functions.
**
** Copyright (C) 2000, 2001 Bilingual Emacspeak Project <inoue@argv.org>
**
** Last modified: $Date: 2002/05/19 13:22:19 $ by $Author: inoue $
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
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "dic.h"
#include "ssdefs.h"
#include "kanjifn.h"

/* convert Japanese jisx0208 alphabet and numeric to ascii */
char
jpZenToHan(char *pc)
{
  unsigned char c1 = (unsigned char)(*pc);
  unsigned char c2 = (unsigned char)(*(pc+1));
  if (c1 == 0x82 && c2 == 0x66)
    return '\'';
  if (c1 != 0x82)
    return '?';
  if (c2 >= 0x4f && c2 <= 0x58)
    return (c2 - 0x4f + '0');
  if (c2 >= 0x60 && c2 <= 0x79)
    return (c2 - 0x60 + 'A');
  if (c2 >= 0x81 && c2 <= 0x9a)
    return (c2 - 0x81 + 'a');
  return '?';
}

int
jpEngToKana(Dictionary *dicp, char *pszIn, int inLen, char **ppszOut)
{
  char *buf = NULL; /* normalized alphabet buffer */
  dicRecord *recBuf[ENG_KANA_MAX_FLAGMENT];
  dicRecord *pdr = NULL;
  dicRecord drWordTail;
  int i, matchFlag;
  int iWords, iBeg, iEnd, iEndTmp;
  char *p, *p2;
  /* list of word tails */
  char *tails[] = {"S", "�Y", "ES", "�C�Y", "\'S", "�Y", "S\'", "�Y\'",
		   "D", "�h�D", "ED", "�h�D", "ING", "�C���O",
		   "LY", "��", NULL};
  *ppszOut = NULL;
#ifdef DEBUG
  fprintf(stderr, "kanjifn: matching ");
  for (int iLoop=0; iLoop < inLen; iLoop++)
    fprintf(stderr, "%c", pszIn[iLoop]);
  fprintf(stderr, "  (%d)\n", inLen);
#endif
  /* Allocate buf. It is always shorter than inLen */
  buf = (char *)malloc(sizeof(char)*inLen+1);
  if (buf == NULL) {
    *ppszOut = NULL;
    return FALSE;
  }
  /* copy to buf capitalizing and hankaku-converting */
  p = pszIn;
  p2 = buf;
  while (p - pszIn < inLen) {
    if (iszenalpha((unsigned char *)p)) {
      *(p2++) = toupper(jpZenToHan(p));
      p += 2;
    }
    else {
      *p2++ = toupper(*p++);
    }
  }
  *p2 = '\0';
#ifdef WRITE_ELOG
  log_error(NULL, "kanjifn: normalized: %s\n", buf);
#endif
  /* find the first matching word conbination */
  matchFlag = 1;
  iWords = 0;
  iBeg = 0;
  iEnd = strlen(buf);
  while (iBeg < iEnd) {
    /* check the rule of word tails */
    if (iBeg > 0) {
      for (i = 0; tails[i] != NULL; i+=2) {
	if (strcmp(tails[i], &buf[iBeg]) != 0)
	  continue;
	drWordTail.key = tails[i];
	drWordTail.cont = tails[i+1];
	drWordTail.attr = 0;
	drWordTail.next = NULL;
	recBuf[iWords++] = &drWordTail;
#ifdef DEBUG
	fprintf(stderr, "kanjifn: %s\n", pdr->key);
#endif
	iBeg = iEnd;
	break;
      }
      if (tails[i] != NULL)
	continue;
    }
    for (iEndTmp = iEnd; iEndTmp > iBeg+1; iEndTmp--) {
      pdr = dicNHashSearch(dicp, &buf[iBeg], iEndTmp-iBeg);
      if (pdr == NULL) continue; /* not found */
      /* If attribute is 1, it is a match only when it is the
	 entire word. */
      if (pdr->attr == 1 && !(iBeg == 0 && iEndTmp == iEnd)) {
	continue;
      }
      /* match found */
      recBuf[iWords++] = pdr;
#ifdef DEBUG
      fprintf(stderr, "%s\n", pdr->key);
#endif
      iBeg = iEndTmp;
      break;
    }
    /* If no match is found in this loop, we don't know this word. */
    if (iEndTmp == iBeg+1) {
      matchFlag = 0;
      break;
    }
  }
  /* buf is not needed anymore */
  if (matchFlag == 0) {
	char *buf2 = NULL;
	if (inLen >= 4 && jpRomaToKana(buf, iEnd, &buf2)) {
	  *ppszOut = buf2;
	  free(buf);
	  return TRUE;
	} else {
	  free(buf);
	  return FALSE;
	}
  }
  free(buf);
  /* count length of converted kana */
  /* Initializer '1' is for the last NULL byte. */
  for (i = 0, iEndTmp = 1; i < iWords; i++) {
    iEndTmp += strlen(recBuf[i]->cont);
  }
  /* allocate new buffer for output */
  buf = (char *)malloc(sizeof(char) * iEndTmp);
  if (buf == NULL) {
    return FALSE;
  }
  /* copy kana string */
  buf[0] = '\0';
  for (i=0; i < iWords; i++) {
#ifdef DEBUG
    fprintf(stderr, "%s\n", recBuf[i]->cont);
#endif
    strcat(buf, recBuf[i]->cont);
  }
  *ppszOut = buf;
  return TRUE;
}

/*
  �ϊ��ꗗ�\
  �A A  �C I  �E U  �G E  �I O
  �J KA  �L KI  �N KU  �P KE  �R KO
  �T SA  �V SI SHI  �X SU  �Z SE  �\ SO
  �^ TA  �` TI CHI  �c TU TSU  �e TE  �g TO
  �i NA  �j NI  �k NU  �l NE  �m NO
  �n HA  �q HI  �t HU FU  �w HE  �z HO
  �} MA  �~ MI  �� MU  �� ME  �� MO
  �� YA  �� YU  �� YO
  �� RA  �� RI  �� RU  �� RE  �� RO
  �� WA  �� WI  �� WE  �� WO
  �� N NN �uB, M, P �̑O�� M�v �u�ꉹ����� Y �̑O�� N'�v
  �[ �uO �̌�� H�v

  �K GA  �M GI  �O GU  �Q GE  �S GO
  �U ZA  �W ZI JI  �Y ZU  �[ ZE  �] ZO
  �_ DA  �a DI  �d DU  �f DE  �h DO
  �o BA  �r BI  �u BU  �x BE  �{ BO
  �p PA  �s PI  �v PU  �y PE  �| PO

  �� VU

  �L�� KYA  �L�B KYI  �L�� KYU  �L�F KYE  �L�� KYO
  �V�� SYA SHA  �V�B SYI  �V�� SYU SHU  �V�F SYE SHE  �V�� SYO SHO
  �`�� TYA CHA  �`�B TYI �`�� TYU CHU  �`�F TYE CHE  �`�� TYO CHO
  �j�� NYA  �j�B NYI  �j�� NYU  �j�F NYE  �j�� NYO
  �q�� HYA  �q�B HYI  �q�� HYU  �q�F HYE  �q�� HYO
  �~�� MYA  �~�B MYI  �~�� MYU  �~�F MYE  �~�� MYO
  ���� RYA  ���B RYI  ���� RYU  ���F RYE  ���� RYO

  �M�� GYA  �M�B GYI  �M�� GYU  �M�F GYE  �M�� GYO
  �W�� ZYA JA JYA  �W�B ZYI JYI  �W�� ZYU JU JYU  �W�F ZYE JE JYE  �W�� ZYO JO JYO
  �a�� DYA  �a�B DYI  �a�� DYU  �a�F DYE  �a�� DYO
  �r�� BYA  �r�B BYI  �r�� BYU  �r�F BYE  �r�� BYO
  ���@ VA  ���B VI  ���F VE  ���H VO
  �s�� PYA  �s�B PYI  �s�� PYU  �s�F PYE  �s�� PYO

  �t�@ FA  �t�B FI  �t�F FE  �t�H FO

  �b�J KKA  �b�L KKI  �b�N KKU  �b�P KKE  �b�R KKO
  �b�T SSA  �b�V SSI SSHI  �b�X SSU  �b�Z SSE  �b�\ SSO
  �b�^ TTA  �b�` TTI CCHI TCHI  �b�c TTU  �b�e TTE  �b�g TTO
  �b�n HHA  �b�q HHI  �b�t HHU FFU  �b�w HHE  �b�z HHO
  �b�t�@ FFA  �b�t�B FFI  �b�t�F FFE  �b�t�H FFO
  �b�� YYA  �b�� YYU  �b�� YYO
  �b�� RRA  �b�� RRI  �b�� RRU  �b�� RRE  �b�� RRO

  �b�K GGA  �b�M GGI  �b�O GGU  �b�Q GGE  �b�S GGO
  �b�U ZZA  �b�W ZZI JJI  �b�Y ZZU  �b�[ ZZE  �b�] ZZO
  �b�_ DDA  �b�a DDI  �b�d DDU  �b�f DDE  �b�h DDO
  �b�o BBA  �b�r BBI  �b�u BBU  �b�x BBE  �b�{ BBO
  �b�p PPA  �b�s PPI  �b�v PPU  �b�y PPE  �b�| PPO

  �b�� VVU

  �b�L�� KKYA  �b�L�B KKYI  �b�L�� KKYU  �b�L�F KKYE  �b�L�� KKYO
  �b�V�� SSYA SSHA  �b�V�B SSYI  �b�V�� SSYU SSHU  �b�V�F SSYE SSHE  �b�V�� SSYO SSHO
  �b�`�� TTYA CCHA TCHA  �b�`�B TTYI  �b�`�� TTYU CCHU TCHU  �b�`�F TTYE CCHE  �b�`�� TTYO CCHO TCHO
  �b�q�� HHYA  �b�q�B HHYI  �b�q�� HHYU  �b�q�F HHYE  �b�q�� HHYO
  �b���� RRYA  �b���B RRYI  �b���� RRYU  �b���F RRYE  �b���� RRYO

  �b�M�� GGYA  �b�M�B GGYI  �b�M�� GGYU  �b�M�F GGYE  �b�M�� GGYO
  �b�W�� ZZYA JJA JJYA  �b�W�B ZZYI JJYI  �b�W�� ZZYU JJU JJYU  �b�W�F ZZYE JJE JJYE  �b�W�� ZZYO JJO JJYO
  �b�a�� DDYA  �b�a�B DDYI  �b�a�� DDYU  �b�a�F DDYE  �b�a�� DDYO
  �b�r�� BBYA  �b�r�B BBYI  �b�r�� BBYU  �b�r�F BBYE  �b�r�� BBYO
  �b���@ VVA  �b���B VVI  �b���F VVE  �b���H VVO
  �b�s�� PPYA  �b�s�B PPYI  �b�s�� PPYU  �b�s�F PPYE  �b�s�� PPYO

  �K��
  o �w�{�����A���{���A�P�ߎ� �S�ĂɑΉ�����
  o ���ꂼ��̕��������݂��Ă����ɋ�ʂ�����舵��
    (�ԈႢ���`�F�b�N�������[�}���ƔF������)
  o �{���R�̋L�q�����ɓ��Ă͂܂�Ȃ���p�ɑ΂��Ă�
    �ǂ�����ԈႦ�͂��̎g�p�����e����
	��FJYA, CCHA
  o ���� - �ɂ͖��Ή�

  �B���ȓ_
  o �u���v�̎�舵��
    kenichi �́u�P���C�`�vor�u�P�j�`�v
  o �����ɂȂ� H �̎�舵��
    OH + �u�ꉹ�v���� �u�I�[�v�u�ꉹ�v�Ƃ͂Ȃ�Ȃ�
  o ������ ������ ... �Ƃ��͕K�v���H
 */
struct roma_tbl_t roma_tbl[][5] = {
  {{"�A", 2}, {"�C", 2}, {"�E", 2}, {"�G", 2}, {"�I", 2}},
  {{"�J", 2}, {"�L", 2}, {"�N", 2}, {"�P", 2}, {"�R", 2}},
  {{"�L��", 4}, {"�L�B", 4}, {"�L��", 4}, {"�L�F", 4}, {"�L��", 4}},
  {{"�T", 2}, {"�V", 2}, {"�X", 2}, {"�Z", 2}, {"\x83\x5c", 2}},
  {{"�V��", 4}, {"�V�B", 4}, {"�V��", 4}, {"�V�F", 4}, {"�V��", 4}},
  {{"�V��", 4}, {"�V", 2}, {"�V��", 4}, {"�V�F", 4}, {"�V��", 4}},
  {{"�^", 2}, {"�`", 2}, {"�c", 2}, {"�e", 2}, {"�g", 2}},
  {{"�`��", 4}, {"�`�B", 4}, {"�`��", 4}, {"�`�F", 4}, {"�`��", 4}},
  {{"�`��", 4}, {"�`", 2}, {"�`��", 4}, {"�`�F", 4}, {"�`��", 4}},
  {{"�i", 2}, {"�j", 2}, {"�k", 2}, {"�l", 2}, {"�m", 2}},
  {{"�j��", 4}, {"�j�B", 4}, {"�j��", 4}, {"�j�F", 4}, {"�j��", 4}},
  {{"�n", 2}, {"�q", 2}, {"�t", 2}, {"�w", 2}, {"�z", 2}},
  {{"�q��", 4}, {"�q�B", 4}, {"�q��", 4}, {"�q�F", 4}, {"�q��", 4}},
  {{"�t�@", 4}, {"�t�B", 4}, {"�t", 2}, {"�t�F", 4}, {"�t�H", 4}},
  {{"�}", 2}, {"�~", 2}, {"��", 2}, {"��", 2}, {"��", 2}},
  {{"�~��", 4}, {"�~�B", 4}, {"�~��", 4}, {"�~�F", 4}, {"�~��", 4}},
  {{"��", 2}, {"��", 2}, {"��", 2}, {"��", 2}, {"��", 2}},
  {{"����", 4}, {"���B", 4}, {"����", 4}, {"���F", 4}, {"����", 4}},
  {{"�K", 2}, {"�M", 2}, {"�O", 2}, {"�Q", 2}, {"�S", 2}},
  {{"�M��", 4}, {"�M�B", 4}, {"�M��", 4}, {"�M�F", 4}, {"�M��", 4}},
  {{"�U", 2}, {"�W", 2}, {"�Y", 2}, {"�[", 2}, {"�]", 2}},
  {{"�W��", 4}, {"�W�B", 4}, {"�W��", 4}, {"�W�F", 4}, {"�W��", 4}},
  {{"�W��", 4}, {"�W", 2}, {"�W��", 4}, {"�W�F", 4}, {"�W��", 4}},
  {{"�W��", 4}, {"�W�B", 4}, {"�W��", 4}, {"�W�F", 4}, {"�W��", 4}},
  {{"�_", 2}, {"�a", 2}, {"�d", 2}, {"�f", 2}, {"�h", 2}},
  {{"�a��", 4}, {"�a�B", 4}, {"�a��", 4}, {"�a�F", 4}, {"�a��", 4}},
  {{"�o", 2}, {"�r", 2}, {"�u", 2}, {"�x", 2}, {"�{", 2}},
  {{"�r��", 4}, {"�r�B", 4}, {"�r��", 4}, {"�r�F", 4}, {"�r��", 4}},
  {{"���@", 4}, {"���B", 4}, {"��", 2}, {"���F", 4}, {"���H", 4}},
  {{"�p", 2}, {"�s", 2}, {"�v", 2}, {"�y", 2}, {"�|", 2}},
  {{"�s��", 4}, {"�s�B", 4}, {"�s��", 4}, {"�s�F", 4}, {"�s��", 4}},
};

int
jpRomaToKana(char *pszIn, int inLen, char **ppszOut)
{
  int last = 0;
  char *buf = NULL;
  char *p;

  buf = (char *)malloc(sizeof(char)*(inLen*ROMA_STR_TIMES+1));
#ifdef DEBUG
  fprintf(stderr, "jpRomaToKana: %s(%d)\n", pszIn, inLen);
#endif

  p = buf;
  p[0] = '\0';
  // Matching romaji string.
  for (int i = 0; i < inLen; i++) {
	if (jpRomaToKana_vowel(pszIn[i], S_A, &p)) {
	  ROMA_ACCEPT;
	} else if ((pszIn[i] == 'K') && ROMA_EXIST_NEXT) {
	  ROMA_NEXT_CHAR;
	  if (jpRomaToKana_vowel(pszIn[i], S_K, &p)) {
		ROMA_ACCEPT;
	  } else if((pszIn[i] == 'Y') && ROMA_EXIST_NEXT) {
		ROMA_NEXT_CHAR;
		if (jpRomaToKana_vowel(pszIn[i], S_KY, &p)) {
		  ROMA_ACCEPT;
		} else {
		  ROMA_FAIL;
		}
	  } else if((pszIn[i] == 'W') && ROMA_EXIST_NEXT) {
		ROMA_NEXT_CHAR;
		if (pszIn[i] == 'A') {
		  ROMA_ACCEPT; strcpy(p, "�N��"); p += 4;
		} else {
		  ROMA_FAIL;
		}
	  } else if(pszIn[i] == 'K') {
		ROMA_ACCEPT; strcpy(p, "�b"); p += 2; i--;
	  } else {
		ROMA_FAIL;
	  }
	} else if ((pszIn[i] == 'S') && ROMA_EXIST_NEXT) {
	  ROMA_NEXT_CHAR;
	  if (jpRomaToKana_vowel(pszIn[i], S_S, &p)) {
		ROMA_ACCEPT;
	  } else if((pszIn[i] == 'Y') && ROMA_EXIST_NEXT) {
		ROMA_NEXT_CHAR;
		if (jpRomaToKana_vowel(pszIn[i], S_SY, &p)) {
		  ROMA_ACCEPT;
		} else {
		  ROMA_FAIL;
		}
	  } else if ((pszIn[i] == 'H') && ROMA_EXIST_NEXT) {
		ROMA_NEXT_CHAR;
		if (jpRomaToKana_vowel(pszIn[i], S_SH, &p)) {
		  ROMA_ACCEPT;
		} else {
		  ROMA_FAIL;
		}
	  } else if(pszIn[i] == 'S') {
		ROMA_ACCEPT; strcpy(p, "�b"); p += 2; i--;
	  } else {
	    ROMA_FAIL;
	  }
	} else if ((pszIn[i] == 'T') && ROMA_EXIST_NEXT) {
	  ROMA_NEXT_CHAR;
	  if (jpRomaToKana_vowel(pszIn[i], S_T, &p)) {
		ROMA_ACCEPT;
	  } else if((pszIn[i] == 'S') && ROMA_EXIST_NEXT) {
		ROMA_NEXT_CHAR;
		if(pszIn[i] == 'U') {
		  ROMA_ACCEPT; strcpy(p, "�c"); p += 2;
		} else {
		  ROMA_FAIL;
		}
	  } else if((pszIn[i] == 'Y') && ROMA_EXIST_NEXT) {
		ROMA_NEXT_CHAR;
		if (jpRomaToKana_vowel(pszIn[i], S_TY, &p)) {
		  ROMA_ACCEPT;
		} else {
		  ROMA_FAIL;
		}
	  } else if(pszIn[i] == 'T') {
		ROMA_ACCEPT; strcpy(p, "�b"); p += 2; i--;
	  } else if(pszIn[i] == 'C') {
		if ((inLen - 2 > i) &&
			(pszIn[i+1] == 'H')) {
		  i += 2;
		  if (pszIn[i] == 'A') {
			ROMA_ACCEPT; strcpy(p, "�b�`��"); p += 6;
		  } else if (pszIn[i] == 'I') {
			ROMA_ACCEPT; strcpy(p, "�b�`"); p += 4;
		  } else if (pszIn[i] == 'U') {
			ROMA_ACCEPT; strcpy(p, "�b�`��"); p += 6;
		  } else if (pszIn[i] == 'O') {
			ROMA_ACCEPT; strcpy(p, "�b�`��"); p += 6;
		  } else {
			ROMA_FAIL;
		  }
		} else {
		  ROMA_FAIL;
		}
	  } else {
		ROMA_FAIL;
	  }
	} else if ((pszIn[i] == 'C') && ROMA_EXIST_NEXT) {
	  ROMA_NEXT_CHAR;
	  if((pszIn[i] == 'H') &&  ROMA_EXIST_NEXT) {
		ROMA_NEXT_CHAR;
		if (jpRomaToKana_vowel(pszIn[i], S_CH, &p)) {
		  ROMA_ACCEPT;
		} else {
		  ROMA_FAIL;
		}
	  } else if(pszIn[i] == 'C') {
		ROMA_ACCEPT; strcpy(p, "�b"); p += 2; i--;
	  } else {
		ROMA_FAIL;
	  }
	} else if (pszIn[i] == 'N') {
	  if (ROMA_EXIST_NEXT) {
		ROMA_NEXT_CHAR;
		if (jpRomaToKana_vowel(pszIn[i], S_N, &p)) {
		  ROMA_ACCEPT;
		} else if((pszIn[i] == 'Y') && ROMA_EXIST_NEXT) {
		  ROMA_NEXT_CHAR;
		  if (jpRomaToKana_vowel(pszIn[i], S_NY, &p)) {
			ROMA_ACCEPT;
		  } else {
			ROMA_FAIL;
		  }
		} else if((inLen - 1 > i) && (pszIn[i] == '\'') &&
				  ((pszIn[i+1] == 'A') || (pszIn[i+1] == 'I') || (pszIn[i+1] == 'U') ||
				   (pszIn[i+1] == 'E') || (pszIn[i+1] == 'O') || (pszIn[i+1] == 'Y'))) {
		  ROMA_ACCEPT; strcpy(p, "��"); p += 2;
		} else if(pszIn[i] == 'N') {
		  ROMA_ACCEPT; strcpy(p, "��"); p += 2;
		} else {
		  ROMA_ACCEPT; strcpy(p, "��"); p += 2; i--;
		}
	  } else {
		ROMA_ACCEPT; strcpy(p, "��");
	  }
	} else if (pszIn[i] == 'H') {
	  if (ROMA_EXIST_NEXT) {
		ROMA_NEXT_CHAR;
		if (jpRomaToKana_vowel(pszIn[i], S_H, &p)) {
		  ROMA_ACCEPT;
		} else if((pszIn[i] == 'Y') && ROMA_EXIST_NEXT) {
		  ROMA_NEXT_CHAR;
		  if (jpRomaToKana_vowel(pszIn[i], S_HY, &p)) {
			ROMA_ACCEPT;
		  } else {
			ROMA_FAIL;
		  }
		} else if(pszIn[i] == 'H') {
		  ROMA_ACCEPT; strcpy(p, "�b"); p += 2; i--;
		} else if((i > 1) && (pszIn[i-2] == 'O')) {
		  ROMA_ACCEPT; strcpy(p, "�["); p += 2; i--;
		} else {
		  ROMA_FAIL;
		}
	  } else if ((i > 0) && (pszIn[i-1] == 'O')){
		ROMA_ACCEPT; strcpy(p, "�["); p += 2;
	  }
	} else if ((pszIn[i] == 'F') && ROMA_EXIST_NEXT) {
	  ROMA_NEXT_CHAR;
	  if (jpRomaToKana_vowel(pszIn[i], S_F, &p)) {
		ROMA_ACCEPT;
	  } else if(pszIn[i] == 'F') {
		ROMA_ACCEPT; strcpy(p, "�b"); p += 2; i--;
	  } else {
		ROMA_FAIL;
	  }
	} else if ((pszIn[i] == 'M') && ROMA_EXIST_NEXT) {
	  ROMA_NEXT_CHAR;
	  if (jpRomaToKana_vowel(pszIn[i], S_M, &p)) {
		ROMA_ACCEPT;
	  } else if((pszIn[i] == 'Y') && ROMA_EXIST_NEXT) {
		ROMA_NEXT_CHAR;
		if (jpRomaToKana_vowel(pszIn[i], S_MY, &p)) {
		  ROMA_ACCEPT;
		} else {
		  ROMA_FAIL;
		}
	  } else if ((pszIn[i] == 'B') || (pszIn[i] == 'M') || (pszIn[i] == 'P')) {
		ROMA_ACCEPT; strcpy(p, "��"); p += 2; i--;
	  } else {
		ROMA_FAIL;
	  }
	} else if ((pszIn[i] == 'Y') && ROMA_EXIST_NEXT) {
	  ROMA_NEXT_CHAR;
	  if (pszIn[i] == 'A') {
		ROMA_ACCEPT; strcpy(p, "��"); p += 2;
	  } else if(pszIn[i] == 'U') {
		ROMA_ACCEPT; strcpy(p, "��"); p += 2;
	  } else if(pszIn[i] == 'O') {
		ROMA_ACCEPT; strcpy(p, "��"); p += 2;
	  } else if(pszIn[i] == 'Y') {
		ROMA_ACCEPT; strcpy(p, "�b"); p += 2; i--;
	  } else {
		ROMA_FAIL;
	  }
	} else if ((pszIn[i] == 'R') && ROMA_EXIST_NEXT) {
	  ROMA_NEXT_CHAR;
	  if (jpRomaToKana_vowel(pszIn[i], S_R, &p)) {
		ROMA_ACCEPT;
	  } else if((pszIn[i] == 'Y') && ROMA_EXIST_NEXT) {
		ROMA_NEXT_CHAR;
		if (jpRomaToKana_vowel(pszIn[i], S_RY, &p)) {
		  ROMA_ACCEPT;
		} else {
		  ROMA_FAIL;
		}
	  } else if(pszIn[i] == 'R') {
		ROMA_ACCEPT; strcpy(p, "�b"); p += 2; i--;
	  } else {
		ROMA_FAIL;
	  }
	} else if ((pszIn[i] == 'W') && ROMA_EXIST_NEXT) {
	  ROMA_NEXT_CHAR;
	  if (pszIn[i] == 'A') {
		ROMA_ACCEPT; strcpy(p, "��"); p += 2;
	  } else if(pszIn[i] == 'I') {
		ROMA_ACCEPT; strcpy(p, "��"); p += 2;
	  } else if(pszIn[i] == 'E') {
		ROMA_ACCEPT; strcpy(p, "��"); p += 2;
	  } else if(pszIn[i] == 'O') {
		ROMA_ACCEPT; strcpy(p, "��"); p += 2;
	  } else {
		ROMA_FAIL;
	  }
	} else if ((pszIn[i] == 'G') && ROMA_EXIST_NEXT) {
	  ROMA_NEXT_CHAR;
	  if (jpRomaToKana_vowel(pszIn[i], S_G, &p)) {
		ROMA_ACCEPT;
	  } else if((pszIn[i] == 'Y') && ROMA_EXIST_NEXT) {
		ROMA_NEXT_CHAR;
		if (jpRomaToKana_vowel(pszIn[i], S_GY, &p)) {
		  ROMA_ACCEPT;
		} else {
		  ROMA_FAIL;
		}
	  } else if((pszIn[i] == 'W') && ROMA_EXIST_NEXT) {
		ROMA_NEXT_CHAR;
		if (pszIn[i] == 'A') {
		  ROMA_ACCEPT; strcpy(p, "�O��"); p += 4;
		} else {
		  ROMA_FAIL;
		}
	  } else if(pszIn[i] == 'G') {
		ROMA_ACCEPT; strcpy(p, "�b"); p += 2; i--;
	  } else {
		ROMA_FAIL;
	  }
	} else if ((pszIn[i] == 'Z') && ROMA_EXIST_NEXT) {
	  ROMA_NEXT_CHAR;
	  if (jpRomaToKana_vowel(pszIn[i], S_Z, &p)) {
		ROMA_ACCEPT;
	  } else if((pszIn[i] == 'Y') && ROMA_EXIST_NEXT) {
		ROMA_NEXT_CHAR;
		if (jpRomaToKana_vowel(pszIn[i], S_ZY, &p)) {
		  ROMA_ACCEPT;
		} else {
		  ROMA_FAIL;
		}
	  } else if(pszIn[i] == 'Z') {
		ROMA_ACCEPT; strcpy(p, "�b"); p += 2; i--;
	  } else {
		ROMA_FAIL;
	  }
	} else if ((pszIn[i] == 'J') && ROMA_EXIST_NEXT) {
	  ROMA_NEXT_CHAR;
	  if (jpRomaToKana_vowel(pszIn[i], S_J, &p)) {
		ROMA_ACCEPT;
	  } else if((pszIn[i] == 'Y') && ROMA_EXIST_NEXT) {
		ROMA_NEXT_CHAR;
		if (jpRomaToKana_vowel(pszIn[i], S_JY, &p)) {
		  ROMA_ACCEPT;
		} else {
		  ROMA_FAIL;
		}
	  } else if(pszIn[i] == 'J') {
		ROMA_ACCEPT; strcpy(p, "�b"); p += 2; i--;
	  } else {
		ROMA_FAIL;
	  }
	} else if ((pszIn[i] == 'D') && ROMA_EXIST_NEXT) {
	  ROMA_NEXT_CHAR;
	  if (jpRomaToKana_vowel(pszIn[i], S_D, &p)) {
		ROMA_ACCEPT;
	  } else if((pszIn[i] == 'Y') && ROMA_EXIST_NEXT) {
		ROMA_NEXT_CHAR;
		if (jpRomaToKana_vowel(pszIn[i], S_DY, &p)) {
		  ROMA_ACCEPT;
		} else {
		  ROMA_FAIL;
		}
	  } else if(pszIn[i] == 'D') {
		ROMA_ACCEPT; strcpy(p, "�b"); p += 2; i--;
	  } else {
		ROMA_FAIL;
	  }
	} else if ((pszIn[i] == 'B') && ROMA_EXIST_NEXT) {
	  ROMA_NEXT_CHAR;
	  if (jpRomaToKana_vowel(pszIn[i], S_B, &p)) {
		ROMA_ACCEPT;
	  } else if((pszIn[i] == 'Y') && ROMA_EXIST_NEXT) {
		ROMA_NEXT_CHAR;
		if (jpRomaToKana_vowel(pszIn[i], S_BY, &p)) {
		  ROMA_ACCEPT;
		} else {
		  ROMA_FAIL;
		}
	  } else if(pszIn[i] == 'B') {
		ROMA_ACCEPT; strcpy(p, "�b"); p += 2; i--;
	  } else {
		ROMA_FAIL;
	  }
	} else if ((pszIn[i] == 'V') && ROMA_EXIST_NEXT) {
	  ROMA_NEXT_CHAR;
	  if (jpRomaToKana_vowel(pszIn[i], S_V, &p)) {
		ROMA_ACCEPT;
	  } else if(pszIn[i] == 'V') {
		ROMA_ACCEPT; strcpy(p, "�b"); p += 2; i--;
	  } else {
		ROMA_FAIL;
	  }
	} else if ((pszIn[i] == 'P') && ROMA_EXIST_NEXT) {
	  ROMA_NEXT_CHAR;
	  if (jpRomaToKana_vowel(pszIn[i], S_P, &p)) {
		ROMA_ACCEPT;
	  } else if((pszIn[i] == 'Y') && ROMA_EXIST_NEXT) {
		ROMA_NEXT_CHAR;
		if (jpRomaToKana_vowel(pszIn[i], S_PY, &p)) {
		  ROMA_ACCEPT;
		} else {
		  ROMA_FAIL;
		}
	  } else if(pszIn[i] == 'P') {
		ROMA_ACCEPT; strcpy(p, "�b"); p += 2; i--;
	  } else {
		ROMA_FAIL;
	  }
	} else {
	  ROMA_FAIL;
	}
  }

  if (last == inLen-1) {
	*ppszOut = buf;
#ifdef DEBUG
	printf("jpRomaToKana: success: %s(%d)\n", *ppszOut, inLen);
#endif
	return TRUE;
  } else {
	ROMA_FAIL;
  }
}

int jpRomaToKana_vowel(char pszIn, int i, char **p)
{
  if (pszIn == 'A') {
	strcpy(*p, roma_tbl[i][0].word);
	*p += roma_tbl[i][0].str_length;
	return TRUE;
  } else if (pszIn == 'I') {
	strcpy(*p, roma_tbl[i][1].word);
	*p += roma_tbl[i][1].str_length;
	return TRUE;
  } else if (pszIn == 'U') {
	strcpy(*p, roma_tbl[i][2].word);
	*p += roma_tbl[i][2].str_length;
	return TRUE;
  } else if (pszIn == 'E') {
	strcpy(*p, roma_tbl[i][3].word);
	*p += roma_tbl[i][3].str_length;
	return TRUE;
  } else if (pszIn == 'O') {
	strcpy(*p, roma_tbl[i][4].word);
	*p += roma_tbl[i][4].str_length;
	return TRUE;
  } else {
	return FALSE;
  }
}
