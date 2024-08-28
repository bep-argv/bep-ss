/* -*- mode: c++; tab-width: 2 -*- */
/*
** dtfunc.cpp --- Voice synthesis functions.
**
** Copyright (C) 2000, 2001 Bilingual Emacspeak Project <inoue@argv.org>
**
** Last modified: $Date: 2002/05/19 13:39:04 $ by $Author: inoue $
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <string>
#include <time.h>
#include "ssdefs.h"
#include "control.h"
#include "dic.h"
#include "kanjifn.h"
#include "dsp.h"
#include "dtvoice.h"
#include "dt.h"
#include "spcsrv.h"
#if SS_SYN_USE_ESD==TRUE
#include <esd.h>
#endif


void *__dt_speak(void *);

dTalkParam::dTalkParam()
{
	ctl.init();
	vset = new DtVoiceSet;
	dsp = NULL;
	status = SS_STAT_NULL;
	lang = SS_LANG_JA;
}

dTalkParam::~dTalkParam()
{
	dicUnloadDictionary(&engKanaDic);
	q.clear();
	delete vset;
	ctl.destroy();
}

void
dTalkParam::setDsp(ssDspDevice *dspp)
{
	dsp = dspp;
}

void
dTalkParam::setStatus(int stat)
{
	ctl.lock();
	status = stat;
	if (status == SS_STAT_STOP_REQUESTED) {
		q.clear();
		dsp->clearLock(this);
		dsp->reset();
	}
	ctl.unlock();
	ctl.cond_broadcast();
}

int
dTalkParam::getStatus()
{
	ctl.lock();
	int stat = status;
	ctl.unlock();
	return stat;
}

int
dTalkParam::setType(string type)
{
	int iRet = 0;
	ctl.lock();
	if (type == "male") {
		iRet = SYT_setVoiceType ( SynHandle, STY_WAVE_KIND_MALE);
	}
	else if (type == "female") {
		iRet = SYT_setVoiceType ( SynHandle, STY_WAVE_KIND_FEMALE);
	}
	ctl.unlock();
	return iRet;
}

int
dTalkParam::setSpeed(int speed)
{
	int iRet;
	ctl.lock();
	iRet = SYT_setSpeed ( SynHandle,
												speed);
	ctl.unlock();
	return iRet;
}

int
dTalkParam::getSpeed()
{
	int iRet;
	ctl.lock();
	iRet = SYT_getSpeed ( SynHandle);
	ctl.unlock();
	return iRet;
}

int
dTalkParam::setPitch(int pitch)
{
	int iRet;
	ctl.lock();
	iRet = SYT_setPitch ( SynHandle,
												pitch);
	ctl.unlock();
	return iRet;
}

int
dTalkParam::getPitch()
{
	int iRet;
	ctl.lock();
	iRet = SYT_getPitch ( SynHandle);
	ctl.unlock();
	return iRet;
}

int
dTalkParam::setPitchRate(int pitchRate)
{
	int iRet;
	ctl.lock();
	iRet = SYT_setIntonation ( SynHandle,
														 pitchRate);
	ctl.unlock();
	return iRet;
}

int
dTalkParam::setTone(int tone)
{
	int iRet;
	ctl.lock();
	iRet = SYT_setTone ( SynHandle,
												tone);
	ctl.unlock();
	return iRet;
}

int
dTalkParam::getTone()
{
	int iRet;
	ctl.lock();
	iRet = SYT_getTone ( SynHandle);
	ctl.unlock();
	return iRet;
}

int
dTalkParam::setVolume(int vol)
{
	int iRet;
	ctl.lock();
	iRet = SYT_setVolume(SynHandle,
											 vol);
	ctl.unlock();
	return iRet;
}

int
dTalkParam::getVolume()
{
	int iRet;
	ctl.lock();
	iRet = SYT_getVolume(SynHandle);
	ctl.unlock();
	return iRet;
}

int
dTalkParam::setPunctuation(int mode)
{
	int dtMode, iRet;
	ctl.lock();
	switch (mode) {
	case SS_PUNCTUATION_ALL:
	case SS_PUNCTUATION_SOME:
		dtMode = LG_SIGN_ON;
		break;
	case SS_PUNCTUATION_NONE:
		dtMode = LG_SIGN_OFF;
		break;
	default:
		dtMode = LG_SIGN_OFF;
	}
	iRet = LNG_setSign ( tLngHandle,
										 dtMode);
	//iRet = LNG_setSign ( tLngHandle,
	//										 LG_SIGN_OFF);
	//LNG_setTextCommand(tLngHandle, LG_TEXTCMD_ON);
	ctl.unlock();
	return iRet;
}

int
dTalkParam::getPunctuation()
{
	short unsigned int dtMode;
	int iRet;
	ctl.lock();
	iRet = LNG_getSign ( tLngHandle, &dtMode);
	ctl.unlock();
	return dtMode;
}

int
dTalkParam::dt_init()
{
	int iRet;
	int iCount;
	int iErrorInfo;
	int failed;

	pszSynDic	 = SS_DTALKER_DIC_DIR "/" SS_DTALKER_SYN_DIC;
	strcpy(szLangDic, SS_DTALKER_DIC_DIR "/" SS_DTALKER_LNG_DIC);
#if 0
	strcpy(szUsrDic1, SS_LNG_DIC_DIRECTORY "/" SS_LNG_DIC_ENG_1);
	strcpy(szUsrDic2, SS_LNG_DIC_DIRECTORY "/" SS_LNG_DIC_ENG_2);
#endif
	SynHandle = NULL;
	iSpeedOffset = 0;	 /* SS全体での速度値と同じ */
	iSpeedFactor = DT_INIT_SPEED_FACTOR;

	/* 英語→カナ辞書のロード */
	char *pszEnvDicPath;
	pszEnvDicPath = getenv("BEP_DIC_PATH");
	if (pszEnvDicPath != NULL
			&& strlen(pszEnvDicPath) < 256) {
		strcpy(szEngKanaDic, pszEnvDicPath);
	} else {
		strcpy(szEngKanaDic, SS_DTALKER_DIC_DIR "/" SS_ENG_KANA_DIC);
	}

	/* load English to Kana dictionary */
	iRet = dicLoadDictionary(&engKanaDic, szEngKanaDic);
	if (iRet != 0) {
		fprintf(stderr, "DT: can not load Eng to Kana dictionary: %s\n",
						szEngKanaDic);
		return SS_OPEN_FAIL;
	}

	failed = 0;

	/*
	 * 言語処理のオープン
	 */
	tLngOpen.pszSysDic = szLangDic;			/* 基本言語辞書ファイル名 */
	for ( iCount = 0; iCount < 15; iCount++ )
		tLngOpen.pszUserDic[ iCount ] = NULL;	/* ユーザー言語辞書は使用しない */
#if 0
	tLngOpen.pszUserDic[0] = szUsrDic1;
	tLngOpen.pszUserDic[1] = szUsrDic2;
#endif
	tLngOpen.wCharSet = LG_CODE_MS;	/* マイクロソフトコード系 */
	tLngOpen.wState = 0;	/* 復帰ステータスをクリア */

	iRet = LNG_analyzeInit ( &tLngHandle, &tLngOpen );
	if ( iRet != LGERR_NOERR )
		{
			fprintf (stderr, "DT: LNG_analyzeInit failed, code %d\n", iRet);
			return SS_OPEN_FAIL;;
		}

	/*
	 * 波形処理のオープン
	 */
	SynHandle = SYT_syntheInit ( pszSynDic, 
															 STY_WAVE_KIND_BOTH, 
															 STY_WAVE_DATA_16LINEAR, 
															 /*STY_WAVE_DATA_8MLAW,*/
															 &iErrorInfo, 
															 "DT_TTS_mmf" );
	if ( SynHandle == NULL )
		{
			fprintf (stderr, "DT: SYT_syntheInit failed\n");
			dt_close();
			return SS_OPEN_FAIL;
		}
	/* Volume */
	//SYT_setVolume(SynHandle, 9);
	/* 英語エンジンとして初期化された場合、ハイトーンになる(暫定措置) */
	if (lang == SS_LANG_EN) {
		SYT_setTone(SynHandle, 1);
		vset->voices[0].type = "female";
	}
	//SYT_setPauseTime(20, 10, 8, 0);
	return SS_OPEN_OK;
}

int
dTalkParam::dt_close()
{
	int iRet;
	int iErrorInfo;
	int failed = 0;

	/*
	 * 言語処理のクローズ
	 */
	iRet = LNG_analyzeEnd ( tLngHandle );
	if ( iRet != LGERR_NOERR )
		{
			fprintf (stderr, "DT: LNG_analyzeEnd failed\n");
			failed++;
		}

	/*
	 * 波形処理のクローズ
	 */
	if (SynHandle != NULL) {
		iErrorInfo = SYT_syntheEnd ( SynHandle );
		if ( iErrorInfo != SYERR_NOERROR )
			{
				fprintf (stderr, "DT: SYT_syntheEnd failed\n");
				failed++;
			}
	}

	if (failed) {
		return SS_CLOSE_FAIL;
	}
	return SS_CLOSE_OK;
}

int
dTalkParam::dt_lang_anal(char *src)
{
	int iRet;

	ctl.lock();
	/*
	 * 言語解析の実行
	 */
	tLng.pszSrc = src;			/* 言語解析 対象文字列 */
	tLng.pszDst = NULL;				/* 言語解析 結果 */
	tLng.wCnvMode = CNV_FIRST | CNV_LAST;		/* 変換モード */
	tLng.wReserved = 0;					/* リザーブ */
	tLng.dwDstLength = 0;				/* 解析結果の長さをクリア */

	iRet = LNG_analyze( tLngHandle, &tLng );
	ctl.unlock();
	if ( iRet != LGERR_NOERR )
		{
			fprintf (stderr, "DT: LNG_analyze failed\n");
			return SS_LANG_ANAL_FAIL;
		}
	
	return SS_LANG_ANAL_OK;
}

void
dTalkParam::dt_do_bracket_command(const char *cmd)
{
	const char *p;
	if (cmd[0] != ':')
		return;
#if DEBUG >= 3
	printf("processing command [%s]\n", cmd);
#endif
	if (strncmp(":name", cmd, 5) == 0) {
		p = cmd+5;
		while(*p == ' ') p++;
		vset->selectVoice(p);
		return;
	}
	if (strncmp(":n", cmd, 2) == 0) {
		switch (cmd[2]) {
		case 'p':
			if (strncmp("p-monotone", &cmd[2], 10)==0)
				vset->selectVoice("paul-monotone");
			else if (strncmp("p-animated", &cmd[2], 10) == 0)
				vset->selectVoice("paul-animated");
			else if (strncmp("p-smooth", &cmd[2], 8) == 0)
				vset->selectVoice("paul-smooth");
			else if (strncmp("p-indent", &cmd[2], 8) == 0)
				vset->selectVoice("indent-voice");
			else vset->selectVoice("paul");
			break;
		case	'b':
			vset->selectVoice("betty");
			break;
		case 'i':
			vset->selectVoice("indent-voice");
			break;
		case 'h':
			vset->selectVoice("harry");
			break;
		case 'u':
			vset->selectVoice("ursula");
			break;
		}
	}
	return;
}

void *
dTalkParam::dt_speak()
{
	int iRet;
	int iErrorInfo;	 /* DTalkerのエラー情報 */
	unsigned long dwGenSize;	 /* 実際に生成されたPCMデータのサイズ */
	char pcPcmBuf[SS_PCMSIZE];
	char *pcEngKanaResult;
	ssRequest req;
	char *pcBeg, *pcEnd, *pcWork, *pcWork2;
	int engFlag, engFlagTmp;
	int iLen;
	string tmpStr, phonStr;
	char dbgstr[10000];

	/* リクエスト取得のためロック */
	ctl.lock();

	/* threadが停止要求を受けるまでループ */
	while(ctl.isactive()) {

		/* キューにリクエストが入るまで待つ */
		while (q.empty()) {
			if (!ctl.isactive()) {
				break;
			}
#if DEBUG >= 3
			fprintf(stderr, "DT: waiting\n");
#endif
			ctl.cond_wait();
		}
		if (!ctl.isactive()) {
			break;
		}
		/* 待ち受け中にstatusが立っていた場合。
			 キューが空になった→停止要求がきた→キューにデータが入った
			 という状況なので、ここでもし停止要求フラグが立っていたら落とす。 */
		status = SS_STAT_NULL;

		/* queueから処理内容を取得。
			 もらった構造体のメモリーはssqueue::push_text()で確保されたもので、
			 もらった人が消去する。*/
		ssRequest *request = (ssRequest *)(q.pop());
		/* requestをコピーし、解放 .
			 これでメモリはスタックに取られるので、ループを抜けた段階で消滅する。*/
		req.dup(request);
		delete request;
		request = NULL;
#if DEBUG >= 3
		fprintf(stderr, "DT: get %s %d\n", req.src.c_str(), req.type);
#endif
#ifdef WRITE_ELOG
		log_error(NULL, "DT: get %s %d\n", req.src.c_str(), req.type);
#endif
		if (req.type == SS_WORK_TYPE_END) {
			/* DSPデバイスを次のエンジンに渡す */
			dsp->releaseLock(this);
#if DEBUG >= 3
			printf("dt: DSP lock released\n");
#endif
			/* ctlをロックしたままでwhile先頭へ */
			continue;
		}
		ctl.unlock();

		/* DSPデバイスを予約して待つ */
		dsp->waitLock(this);
		/* 出力デバイスのオープンを確認 */
		dsp->open();

		char src[req.src.size() + 1];
		strcpy(src, req.src.c_str());
		strcpy(dbgstr, src);
		/* 発声速度の補正。*/
		req.iSpeed += iSpeedOffset;
		req.iSpeed /= iSpeedFactor;
		if (req.iSpeed > DT_MAX_SPEED) {
			req.iSpeed = DT_MAX_SPEED;
		}

		/* ポインタの初期化 */
		pcBeg = src;
		iLen = strlen(src);

		/* 文字列が終わるまでループ */
		while(1) {
			/* 先頭のスペースを除去 */
			while(*pcBeg == ' ' && *pcBeg != '\0')
				pcBeg++;
#if DEBUG >= 3
			printf("remaining: %s\n", pcBeg);
#endif
			/* pcBeg, pcEndは同じ位置 */
			pcEnd = pcBeg;

			/* 文字列の終了チェック */
			if (*pcBeg == '\0') {
				break;
			}

			if (*pcBeg == '[') {
				/* DECTalk風in-text-commandを処理し、ポインタを進める */
				/* pcEndでコマンドの終わりを探してそこに\0をセット*/
				while (*pcEnd != ']' && *pcEnd != '\0')
					pcEnd++;
				*pcEnd = '\0';
				/* コマンドを解釈 */
				dt_do_bracket_command(pcBeg+1);
				/* pcBegをNULL文字のところへ */
				pcBeg = pcEnd;
				/*文字列の最後チェック、ループ脱出 */
				if (pcBeg - src >= iLen)
					break;
				/* pcBegをNULL文字の次へ移動してループ最初へ */
				pcBeg++;
				continue;
			}

			/* コマンドでない文字列の範囲を取得 */
			while (*pcEnd != '[' && *pcEnd != '\0'
						 && pcEnd - src < iLen) {
				if (iskanji(*pcEnd)) pcEnd+=2;
				else pcEnd++;
			}

			/* 半角/全角の英数字の範囲を取得 */
			tmpStr = "";
			pcWork = pcBeg;
			pcWork2 = pcWork;
			engFlag = FALSE;
			while (pcWork <= pcEnd) {
				if ((pcWork != pcEnd)
						&& (iszenalpha((unsigned char *)pcWork) || isalpha(*pcWork)
								|| *pcWork == '\''))
					engFlagTmp = TRUE;
				else engFlagTmp = FALSE;
				if (pcBeg == pcWork) { /* if pcWork points the first char */
					engFlag = engFlagTmp;
					if (iskanji(*pcWork)) pcWork+=2;
					else pcWork++;
					continue;
				}
				/* 英数字列の終わりか文字列の終わりなら英語→カナ変換 */
				if (engFlag != engFlagTmp || pcWork == pcEnd) {
					if (engFlag == TRUE) {
						if (jpEngToKana(&engKanaDic, pcWork2,
														(pcWork-pcWork2), &pcEngKanaResult)
								) {
#if DEBUG >= 3
							printf("converted: %s\n", pcEngKanaResult);
#endif
							/* 変換結果をappend */
							tmpStr.append(pcEngKanaResult);
							/* jpEngToKana()がmalloc()したメモリを解放 */
							free (pcEngKanaResult);
						} else {
							/* 変換できなかったとき、英字の間にスペースを挿入して追加 */
#if WRITE_ELOG
							log_error(NULL, "DT: append with space\n");
#endif
							for (char *p = pcWork2; p < pcWork; p++) {
								if (iszenalpha((unsigned char*)p)) {
									tmpStr.append(p, 2);
									p++;
								} else {
									tmpStr.append(p, 1);
								}
								if (p < pcWork - 1)
									tmpStr.append(" ");
							}
						}
					}	else {
						/* 英数字列でないか、カタカナに変換できなかったとき
							 そのままappend */
						if ((pcWork2 != pcBeg && !isdigit(*(pcWork2-1)))
								&& *pcWork2 == ' ')
							pcWork2++;
						tmpStr.append(pcWork2, (pcWork-pcWork2));
					}
					pcWork2 = pcWork;
					engFlag = engFlagTmp;
				}
				if (pcWork == pcEnd) break;
				/* goto next char */
				if (iskanji(*pcWork)) pcWork+=2;
				else pcWork++;
			} /* 文字列をtmpstrに入れる処理終了 */

			// tmpstrに対して読みの置換処理
			replace_reading(tmpStr, req.punctuation);

			/* 音声合成処理へ */
#if DEBUG >= 3
			printf("speaking: %s\n", tmpStr.c_str());
#endif
			pcBeg = pcEnd;

			/* モード設定 */
			iErrorInfo = setSpeed (req.iSpeed);
			if (iErrorInfo != SYERR_NOERROR)
				{
					fprintf(stderr, "DT: SYT_setSpeed() failed\n");
					goto EXIT_FAIL;
				}

			iErrorInfo = setPunctuation (req.punctuation);
			if (iErrorInfo != SYERR_NOERROR)
				{
					fprintf(stderr, "DT: SYT_setSpeed() failed\n");
					goto EXIT_FAIL;
				}

			/* 言語解析 */
			iRet = dt_lang_anal((char *)tmpStr.c_str());
			if (iRet != SS_LANG_ANAL_OK) {
				goto EXIT_FAIL;
			}
			/* 変換後表音文字列が空の場合はループ先頭へ */
			if (tLng.pszDst == NULL) {
				continue;
			}
	
			/* 現在の音声設定を波形処理に反映 */
			setVolume(vset->volume());
			setType(vset->type());
			setPitch(vset->pitch());
			setPitchRate(vset->pitchRate());
			setTone(vset->tone());
			/* 文字読みモードで大文字ならピッチを上げる。 */
			{
				unsigned char tmpc[2];
				tmpc[0] = tmpStr.c_str()[0];
				tmpc[1] = tmpStr.c_str()[1];
				if (req.type == SS_WORK_TYPE_LETTER
						&& (isupper(tmpc[0])
								|| iskatakana(tmpc)) ) {
					setPitch (vset->pitch()+2);
				}
			}

			/*
			 * 表音文字列をPCMデータに変換する
			 */

#if DEBUG >= 3
			fprintf(stderr, "DT: Synthesis: %s\n", tLng.pszDst);
#endif
#ifdef WRITE_ELOG
			log_error(NULL, "DT: Synthesis: %s\n", tLng.pszDst);
#endif
			iErrorInfo = SYT_generatePcmStart ( SynHandle,
																					tLng.pszDst,
																					0);
			if ( iErrorInfo != SYERR_NOERROR )
				{
					fprintf (stderr, "DT: SYT_generatePcmStart failed, %d\n",
									 iErrorInfo);
					goto EXIT_FAIL;
				}

			/* サウンドデバイスに出力 */
			for ( ; ; ) {
				/* check stop request */
				if (isStop()) {
					status = SS_STAT_NULL;
					goto STOP_OK;
				}
				/* 指定サイズ分のPCMデータ生成 */
				iErrorInfo = SYT_generatePcm ( SynHandle, 
																			 pcPcmBuf,
																			 SS_PCMSIZE, 
																			 &dwGenSize );
				if ( iErrorInfo == SYRET_CONTINUE || iErrorInfo == SYRET_END )
					{
						/* 間引き処理をしてサウンドデバイスへ */
						if (iErrorInfo == SYRET_CONTINUE) {
							int size;
#if 1
							size = ssWarpPcm((short *)pcPcmBuf, dwGenSize/2, 0.95);
#else
							size = ssSkipPcm((short *)pcPcmBuf, dwGenSize/2, SS_SKIP_UNIT, SS_SKIP_FRAMES);
#endif
							/* PCMデータをオーディオデバイスに書き込む */
							if ( dsp->write ( pcPcmBuf,
																size*2 ) != size*2 )
								{
									fprintf (stderr, "DT: cannot write to audio\n");
								}
						}
						/* もう生成するPCMデータは残っていない */
						if ( iErrorInfo == SYRET_END ) {
							memset(pcPcmBuf, 0, SS_PCMSIZE);
							dsp->write(pcPcmBuf, SS_PCMSIZE);
							break;
						}
					}
				else
					{
						/* PCMデータの生成に失敗した */
						fprintf (stderr, "DT: SYT_generatePcm failed\n");
						goto EXIT_FAIL;
					}
			}
		}
#if DEBUG >= 3
		printf("output complete\n");
#endif
		/* リクエスト取得のためロック */
		ctl.lock();
		continue;

	STOP_OK:
		SYT_generatePcmStop(SynHandle);
#if DEBUG >= 3
		printf("stop request accepted\n");
#endif
		vset->selectVoice("paul");
		/* リクエスト取得のためロック */
		ctl.lock();
	}
	ctl.unlock();
	return 0;

 EXIT_FAIL:
	dt_close();
	return 0;
}

int
dTalkParam::start()
{
	int ret;
	/* 音声合成の初期化 */
	fprintf(stderr, "DT: Initializing CreateSystem speech engine...\n");
	int iErrorInfo = dt_init();
	if (iErrorInfo != SS_OPEN_OK) {
		fprintf(stderr, "\nDT: initialization failed\n");
		return -1;
	} else {
#if DEBUG >= 3
		fprintf(stderr, "done\n");
#endif
	}
	ctl.activate();
	ret = pthread_create(&th, NULL, &__dt_speak, (void *)this);
	return ret;
}

int
dTalkParam::shutdown()
{
#if DEBUG >= 3
	fprintf(stderr, "DT: shutdown\n");
#endif
	ctl.deactivate();
	ctl.cond_broadcast();
	pthread_join(th, NULL);
	dt_close();
	return 0;
}

int
dTalkParam::add(ssRequest *req)
{
	ctl.lock();
	q.push((void *)req);
	ctl.unlock();
	return 0;
}

int
dTalkParam::speak()
{
	ssRequest *req = new ssRequest();
	req->type = SS_WORK_TYPE_END;
	/* DSPデバイスの順番を予約 */
	dsp->reserveLock(this);
	ctl.lock();
	q.push((void *)req);
	ctl.unlock();
	ctl.cond_broadcast();
	return 0;
}

/* strの中の指定した記号類を読みに置き換える。
 読みはdt.hにSJISの文字列定数として定義されている。
 modeはpunctuation modeで、これによって動作が異なる。*/
int
dTalkParam::replace_reading(string& str, int mode)
{
	int matches = 0;
	size_t pos, match;
	char *char_to_replace = "@()\'\""; // 置換する対象の文字をここにならべる。
	pos = match = 0;
	while(pos < str.length()) {
		match = str.find_first_of(char_to_replace, pos);
		if (match >= str.length())
			break;
		if (match > 0 && iskanji(str[match - 1])) {
			pos = match + 1;
			continue;
		}			
		matches++;
		if (mode == SS_PUNCTUATION_ALL
				|| mode == SS_PUNCTUATION_SOME) {
			switch (str[match]) {
			case '@':
				str.replace(match, 1, JA_AT_SJIS);
				pos = match + strlen(JA_AT_SJIS);
				break;
			case '(':
				str.replace(match, 1, JA_OPEN_PAREN_SJIS);
				pos = match + strlen(JA_OPEN_PAREN_SJIS);
				break;
			case ')':
				str.replace(match, 1, JA_CLOSE_PAREN_SJIS);
				pos = match + strlen(JA_CLOSE_PAREN_SJIS);
				break;
			case '\'':
				str.replace(match, 1, JA_APOS_SJIS);
				pos = match + strlen(JA_APOS_SJIS);
				break;
			case '\"':
				str.replace(match, 1, JA_QUOTE_SJIS);
				pos = match + strlen(JA_QUOTE_SJIS);
				break;
#if 0
			case ',':
				str.replace(match, 1, "\x81\x97\x82\x4f,");
				pos = match + 4;
				break;
			case '.':
				str.replace(match, 1, "\x81\x97\x82\x4f.");
				pos = match + 4;
				break;
			case '/':
				str.replace(match, 1, "\x81\x97\x82\x4f/");
				pos = match + 4;
				break;
			case ' ':
				str.replace(match, 1, "\x81\x97\x82\x5b ");
				pos = match + 4;
				break;
#endif
			default:
				if (iskanji(str[match])) pos = match + 2;
				else pos = match + 1;
			}
		}
		else {
			str.replace(match, 1, " ");
			pos = match + 1;
		}
	}
	return matches;
}

/* thread開始のためのラッパー。 */
void *
__dt_speak(void *arg)
{
	dTalkParam *ss = (dTalkParam *)arg;
	ss->dt_speak();
	return 0;
}
