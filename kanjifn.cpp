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
  char *tails[] = {"S", "ズ", "ES", "イズ", "\'S", "ズ", "S\'", "ズ\'",
		   "D", "ドゥ", "ED", "ドゥ", "ING", "イング",
		   "LY", "リ", NULL};
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
  変換一覧表
  ア A  イ I  ウ U  エ E  オ O
  カ KA  キ KI  ク KU  ケ KE  コ KO
  サ SA  シ SI SHI  ス SU  セ SE  ソ SO
  タ TA  チ TI CHI  ツ TU TSU  テ TE  ト TO
  ナ NA  ニ NI  ヌ NU  ネ NE  ノ NO
  ハ HA  ヒ HI  フ HU FU  ヘ HE  ホ HO
  マ MA  ミ MI  ム MU  メ ME  モ MO
  ヤ YA  ユ YU  ヨ YO
  ラ RA  リ RI  ル RU  レ RE  ロ RO
  ワ WA  ヰ WI  ヱ WE  ヲ WO
  ン N NN 「B, M, P の前の M」 「母音および Y の前の N'」
  ー 「O の後の H」

  ガ GA  ギ GI  グ GU  ゲ GE  ゴ GO
  ザ ZA  ジ ZI JI  ズ ZU  ゼ ZE  ゾ ZO
  ダ DA  ヂ DI  ヅ DU  デ DE  ド DO
  バ BA  ビ BI  ブ BU  ベ BE  ボ BO
  パ PA  ピ PI  プ PU  ペ PE  ポ PO

  ヴ VU

  キャ KYA  キィ KYI  キュ KYU  キェ KYE  キョ KYO
  シャ SYA SHA  シィ SYI  シュ SYU SHU  シェ SYE SHE  ショ SYO SHO
  チャ TYA CHA  チィ TYI チュ TYU CHU  チェ TYE CHE  チョ TYO CHO
  ニャ NYA  ニィ NYI  ニュ NYU  ニェ NYE  ニョ NYO
  ヒャ HYA  ヒィ HYI  ヒュ HYU  ヒェ HYE  ヒョ HYO
  ミャ MYA  ミィ MYI  ミュ MYU  ミェ MYE  ミョ MYO
  リャ RYA  リィ RYI  リュ RYU  リェ RYE  リョ RYO

  ギャ GYA  ギィ GYI  ギュ GYU  ギェ GYE  ギョ GYO
  ジャ ZYA JA JYA  ジィ ZYI JYI  ジュ ZYU JU JYU  ジェ ZYE JE JYE  ジョ ZYO JO JYO
  ヂャ DYA  ヂィ DYI  ヂュ DYU  ヂェ DYE  ヂョ DYO
  ビャ BYA  ビィ BYI  ビュ BYU  ビェ BYE  ビョ BYO
  ヴァ VA  ヴィ VI  ヴェ VE  ヴォ VO
  ピャ PYA  ピィ PYI  ピュ PYU  ピェ PYE  ピョ PYO

  ファ FA  フィ FI  フェ FE  フォ FO

  ッカ KKA  ッキ KKI  ック KKU  ッケ KKE  ッコ KKO
  ッサ SSA  ッシ SSI SSHI  ッス SSU  ッセ SSE  ッソ SSO
  ッタ TTA  ッチ TTI CCHI TCHI  ッツ TTU  ッテ TTE  ット TTO
  ッハ HHA  ッヒ HHI  ッフ HHU FFU  ッヘ HHE  ッホ HHO
  ッファ FFA  ッフィ FFI  ッフェ FFE  ッフォ FFO
  ッヤ YYA  ッユ YYU  ッヨ YYO
  ッラ RRA  ッリ RRI  ッル RRU  ッレ RRE  ッロ RRO

  ッガ GGA  ッギ GGI  ッグ GGU  ッゲ GGE  ッゴ GGO
  ッザ ZZA  ッジ ZZI JJI  ッズ ZZU  ッゼ ZZE  ッゾ ZZO
  ッダ DDA  ッヂ DDI  ッヅ DDU  ッデ DDE  ッド DDO
  ッバ BBA  ッビ BBI  ッブ BBU  ッベ BBE  ッボ BBO
  ッパ PPA  ッピ PPI  ップ PPU  ッペ PPE  ッポ PPO

  ッヴ VVU

  ッキャ KKYA  ッキィ KKYI  ッキュ KKYU  ッキェ KKYE  ッキョ KKYO
  ッシャ SSYA SSHA  ッシィ SSYI  ッシュ SSYU SSHU  ッシェ SSYE SSHE  ッショ SSYO SSHO
  ッチャ TTYA CCHA TCHA  ッチィ TTYI  ッチュ TTYU CCHU TCHU  ッチェ TTYE CCHE  ッチョ TTYO CCHO TCHO
  ッヒャ HHYA  ッヒィ HHYI  ッヒュ HHYU  ッヒェ HHYE  ッヒョ HHYO
  ッリャ RRYA  ッリィ RRYI  ッリュ RRYU  ッリェ RRYE  ッリョ RRYO

  ッギャ GGYA  ッギィ GGYI  ッギュ GGYU  ッギェ GGYE  ッギョ GGYO
  ッジャ ZZYA JJA JJYA  ッジィ ZZYI JJYI  ッジュ ZZYU JJU JJYU  ッジェ ZZYE JJE JJYE  ッジョ ZZYO JJO JJYO
  ッヂャ DDYA  ッヂィ DDYI  ッヂュ DDYU  ッヂェ DDYE  ッヂョ DDYO
  ッビャ BBYA  ッビィ BBYI  ッビュ BBYU  ッビェ BBYE  ッビョ BBYO
  ッヴァ VVA  ッヴィ VVI  ッヴェ VVE  ッヴォ VVO
  ッピャ PPYA  ッピィ PPYI  ッピュ PPYU  ッピェ PPYE  ッピョ PPYO

  規則
  o ヘボン式、日本式、訓令式 全てに対応する
  o それぞれの方式が混在しても特に区別せず取り扱う
    (間違いをチェックせずローマ字と認識する)
  o 本来３つの記述方式に当てはまらない誤用に対しても
    良くある間違えはその使用を許容する
	例：JYA, CCHA
  o 長音 - には未対応

  曖昧な点
  o 「ン」の取り扱い
    kenichi は「ケンイチ」or「ケニチ」
  o 長音になる H の取り扱い
    OH + 「母音」だと 「オー」「母音」とはならない
  o 小さい ぁぃぅ ... とかは必要か？
 */
struct roma_tbl_t roma_tbl[][5] = {
  {{"ア", 2}, {"イ", 2}, {"ウ", 2}, {"エ", 2}, {"オ", 2}},
  {{"カ", 2}, {"キ", 2}, {"ク", 2}, {"ケ", 2}, {"コ", 2}},
  {{"キャ", 4}, {"キィ", 4}, {"キュ", 4}, {"キェ", 4}, {"キョ", 4}},
  {{"サ", 2}, {"シ", 2}, {"ス", 2}, {"セ", 2}, {"\x83\x5c", 2}},
  {{"シャ", 4}, {"シィ", 4}, {"シュ", 4}, {"シェ", 4}, {"ショ", 4}},
  {{"シャ", 4}, {"シ", 2}, {"シュ", 4}, {"シェ", 4}, {"ショ", 4}},
  {{"タ", 2}, {"チ", 2}, {"ツ", 2}, {"テ", 2}, {"ト", 2}},
  {{"チャ", 4}, {"チィ", 4}, {"チュ", 4}, {"チェ", 4}, {"チョ", 4}},
  {{"チャ", 4}, {"チ", 2}, {"チュ", 4}, {"チェ", 4}, {"チョ", 4}},
  {{"ナ", 2}, {"ニ", 2}, {"ヌ", 2}, {"ネ", 2}, {"ノ", 2}},
  {{"ニャ", 4}, {"ニィ", 4}, {"ニュ", 4}, {"ニェ", 4}, {"ニョ", 4}},
  {{"ハ", 2}, {"ヒ", 2}, {"フ", 2}, {"ヘ", 2}, {"ホ", 2}},
  {{"ヒャ", 4}, {"ヒィ", 4}, {"ヒュ", 4}, {"ヒェ", 4}, {"ヒョ", 4}},
  {{"ファ", 4}, {"フィ", 4}, {"フ", 2}, {"フェ", 4}, {"フォ", 4}},
  {{"マ", 2}, {"ミ", 2}, {"ム", 2}, {"メ", 2}, {"モ", 2}},
  {{"ミャ", 4}, {"ミィ", 4}, {"ミュ", 4}, {"ミェ", 4}, {"ミョ", 4}},
  {{"ラ", 2}, {"リ", 2}, {"ル", 2}, {"レ", 2}, {"ロ", 2}},
  {{"リャ", 4}, {"リィ", 4}, {"リュ", 4}, {"リェ", 4}, {"リョ", 4}},
  {{"ガ", 2}, {"ギ", 2}, {"グ", 2}, {"ゲ", 2}, {"ゴ", 2}},
  {{"ギャ", 4}, {"ギィ", 4}, {"ギュ", 4}, {"ギェ", 4}, {"ギョ", 4}},
  {{"ザ", 2}, {"ジ", 2}, {"ズ", 2}, {"ゼ", 2}, {"ゾ", 2}},
  {{"ジャ", 4}, {"ジィ", 4}, {"ジュ", 4}, {"ジェ", 4}, {"ジョ", 4}},
  {{"ジャ", 4}, {"ジ", 2}, {"ジュ", 4}, {"ジェ", 4}, {"ジョ", 4}},
  {{"ジャ", 4}, {"ジィ", 4}, {"ジュ", 4}, {"ジェ", 4}, {"ジョ", 4}},
  {{"ダ", 2}, {"ヂ", 2}, {"ヅ", 2}, {"デ", 2}, {"ド", 2}},
  {{"ヂャ", 4}, {"ヂィ", 4}, {"ヂュ", 4}, {"ヂェ", 4}, {"ヂョ", 4}},
  {{"バ", 2}, {"ビ", 2}, {"ブ", 2}, {"ベ", 2}, {"ボ", 2}},
  {{"ビャ", 4}, {"ビィ", 4}, {"ビュ", 4}, {"ビェ", 4}, {"ビョ", 4}},
  {{"ヴァ", 4}, {"ヴィ", 4}, {"ヴ", 2}, {"ヴェ", 4}, {"ヴォ", 4}},
  {{"パ", 2}, {"ピ", 2}, {"プ", 2}, {"ペ", 2}, {"ポ", 2}},
  {{"ピャ", 4}, {"ピィ", 4}, {"ピュ", 4}, {"ピェ", 4}, {"ピョ", 4}},
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
		  ROMA_ACCEPT; strcpy(p, "クヮ"); p += 4;
		} else {
		  ROMA_FAIL;
		}
	  } else if(pszIn[i] == 'K') {
		ROMA_ACCEPT; strcpy(p, "ッ"); p += 2; i--;
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
		ROMA_ACCEPT; strcpy(p, "ッ"); p += 2; i--;
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
		  ROMA_ACCEPT; strcpy(p, "ツ"); p += 2;
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
		ROMA_ACCEPT; strcpy(p, "ッ"); p += 2; i--;
	  } else if(pszIn[i] == 'C') {
		if ((inLen - 2 > i) &&
			(pszIn[i+1] == 'H')) {
		  i += 2;
		  if (pszIn[i] == 'A') {
			ROMA_ACCEPT; strcpy(p, "ッチャ"); p += 6;
		  } else if (pszIn[i] == 'I') {
			ROMA_ACCEPT; strcpy(p, "ッチ"); p += 4;
		  } else if (pszIn[i] == 'U') {
			ROMA_ACCEPT; strcpy(p, "ッチュ"); p += 6;
		  } else if (pszIn[i] == 'O') {
			ROMA_ACCEPT; strcpy(p, "ッチョ"); p += 6;
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
		ROMA_ACCEPT; strcpy(p, "ッ"); p += 2; i--;
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
		  ROMA_ACCEPT; strcpy(p, "ン"); p += 2;
		} else if(pszIn[i] == 'N') {
		  ROMA_ACCEPT; strcpy(p, "ン"); p += 2;
		} else {
		  ROMA_ACCEPT; strcpy(p, "ン"); p += 2; i--;
		}
	  } else {
		ROMA_ACCEPT; strcpy(p, "ン");
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
		  ROMA_ACCEPT; strcpy(p, "ッ"); p += 2; i--;
		} else if((i > 1) && (pszIn[i-2] == 'O')) {
		  ROMA_ACCEPT; strcpy(p, "ー"); p += 2; i--;
		} else {
		  ROMA_FAIL;
		}
	  } else if ((i > 0) && (pszIn[i-1] == 'O')){
		ROMA_ACCEPT; strcpy(p, "ー"); p += 2;
	  }
	} else if ((pszIn[i] == 'F') && ROMA_EXIST_NEXT) {
	  ROMA_NEXT_CHAR;
	  if (jpRomaToKana_vowel(pszIn[i], S_F, &p)) {
		ROMA_ACCEPT;
	  } else if(pszIn[i] == 'F') {
		ROMA_ACCEPT; strcpy(p, "ッ"); p += 2; i--;
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
		ROMA_ACCEPT; strcpy(p, "ン"); p += 2; i--;
	  } else {
		ROMA_FAIL;
	  }
	} else if ((pszIn[i] == 'Y') && ROMA_EXIST_NEXT) {
	  ROMA_NEXT_CHAR;
	  if (pszIn[i] == 'A') {
		ROMA_ACCEPT; strcpy(p, "ヤ"); p += 2;
	  } else if(pszIn[i] == 'U') {
		ROMA_ACCEPT; strcpy(p, "ユ"); p += 2;
	  } else if(pszIn[i] == 'O') {
		ROMA_ACCEPT; strcpy(p, "ヨ"); p += 2;
	  } else if(pszIn[i] == 'Y') {
		ROMA_ACCEPT; strcpy(p, "ッ"); p += 2; i--;
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
		ROMA_ACCEPT; strcpy(p, "ッ"); p += 2; i--;
	  } else {
		ROMA_FAIL;
	  }
	} else if ((pszIn[i] == 'W') && ROMA_EXIST_NEXT) {
	  ROMA_NEXT_CHAR;
	  if (pszIn[i] == 'A') {
		ROMA_ACCEPT; strcpy(p, "ワ"); p += 2;
	  } else if(pszIn[i] == 'I') {
		ROMA_ACCEPT; strcpy(p, "ヰ"); p += 2;
	  } else if(pszIn[i] == 'E') {
		ROMA_ACCEPT; strcpy(p, "ヱ"); p += 2;
	  } else if(pszIn[i] == 'O') {
		ROMA_ACCEPT; strcpy(p, "ヲ"); p += 2;
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
		  ROMA_ACCEPT; strcpy(p, "グヮ"); p += 4;
		} else {
		  ROMA_FAIL;
		}
	  } else if(pszIn[i] == 'G') {
		ROMA_ACCEPT; strcpy(p, "ッ"); p += 2; i--;
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
		ROMA_ACCEPT; strcpy(p, "ッ"); p += 2; i--;
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
		ROMA_ACCEPT; strcpy(p, "ッ"); p += 2; i--;
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
		ROMA_ACCEPT; strcpy(p, "ッ"); p += 2; i--;
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
		ROMA_ACCEPT; strcpy(p, "ッ"); p += 2; i--;
	  } else {
		ROMA_FAIL;
	  }
	} else if ((pszIn[i] == 'V') && ROMA_EXIST_NEXT) {
	  ROMA_NEXT_CHAR;
	  if (jpRomaToKana_vowel(pszIn[i], S_V, &p)) {
		ROMA_ACCEPT;
	  } else if(pszIn[i] == 'V') {
		ROMA_ACCEPT; strcpy(p, "ッ"); p += 2; i--;
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
		ROMA_ACCEPT; strcpy(p, "ッ"); p += 2; i--;
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
