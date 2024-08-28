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
	iSpeedOffset = 0;	 /* SS���ΤǤ�®���ͤ�Ʊ�� */
	iSpeedFactor = DT_INIT_SPEED_FACTOR;

	/* �Ѹ좪���ʼ���Υ��� */
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
	 * ��������Υ����ץ�
	 */
	tLngOpen.pszSysDic = szLangDic;			/* ���ܸ��켭��ե�����̾ */
	for ( iCount = 0; iCount < 15; iCount++ )
		tLngOpen.pszUserDic[ iCount ] = NULL;	/* �桼�������켭��ϻ��Ѥ��ʤ� */
#if 0
	tLngOpen.pszUserDic[0] = szUsrDic1;
	tLngOpen.pszUserDic[1] = szUsrDic2;
#endif
	tLngOpen.wCharSet = LG_CODE_MS;	/* �ޥ������եȥ����ɷ� */
	tLngOpen.wState = 0;	/* �������ơ������򥯥ꥢ */

	iRet = LNG_analyzeInit ( &tLngHandle, &tLngOpen );
	if ( iRet != LGERR_NOERR )
		{
			fprintf (stderr, "DT: LNG_analyzeInit failed, code %d\n", iRet);
			return SS_OPEN_FAIL;;
		}

	/*
	 * �ȷ������Υ����ץ�
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
	/* �Ѹ쥨�󥸥�Ȥ��ƽ�������줿��硢�ϥ��ȡ���ˤʤ�(��������) */
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
	 * ��������Υ�����
	 */
	iRet = LNG_analyzeEnd ( tLngHandle );
	if ( iRet != LGERR_NOERR )
		{
			fprintf (stderr, "DT: LNG_analyzeEnd failed\n");
			failed++;
		}

	/*
	 * �ȷ������Υ�����
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
	 * ������Ϥμ¹�
	 */
	tLng.pszSrc = src;			/* ������� �о�ʸ���� */
	tLng.pszDst = NULL;				/* ������� ��� */
	tLng.wCnvMode = CNV_FIRST | CNV_LAST;		/* �Ѵ��⡼�� */
	tLng.wReserved = 0;					/* �ꥶ���� */
	tLng.dwDstLength = 0;				/* ���Ϸ�̤�Ĺ���򥯥ꥢ */

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
	int iErrorInfo;	 /* DTalker�Υ��顼���� */
	unsigned long dwGenSize;	 /* �ºݤ��������줿PCM�ǡ����Υ����� */
	char pcPcmBuf[SS_PCMSIZE];
	char *pcEngKanaResult;
	ssRequest req;
	char *pcBeg, *pcEnd, *pcWork, *pcWork2;
	int engFlag, engFlagTmp;
	int iLen;
	string tmpStr, phonStr;
	char dbgstr[10000];

	/* �ꥯ�����ȼ����Τ����å� */
	ctl.lock();

	/* thread������׵�������ޤǥ롼�� */
	while(ctl.isactive()) {

		/* ���塼�˥ꥯ�����Ȥ�����ޤ��Ԥ� */
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
		/* �Ԥ��������status��Ω�äƤ�����硣
			 ���塼�����ˤʤä�������׵᤬���������塼�˥ǡ��������ä�
			 �Ȥ��������ʤΤǡ������Ǥ⤷����׵�ե饰��Ω�äƤ�������Ȥ��� */
		status = SS_STAT_NULL;

		/* queue����������Ƥ������
			 ���ä���¤�ΤΥ��꡼��ssqueue::push_text()�ǳ��ݤ��줿��Τǡ�
			 ���ä��ͤ��õ�롣*/
		ssRequest *request = (ssRequest *)(q.pop());
		/* request�򥳥ԡ��������� .
			 ����ǥ���ϥ����å��˼����Τǡ��롼�פ�ȴ�����ʳ��Ǿ��Ǥ��롣*/
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
			/* DSP�ǥХ����򼡤Υ��󥸥���Ϥ� */
			dsp->releaseLock(this);
#if DEBUG >= 3
			printf("dt: DSP lock released\n");
#endif
			/* ctl���å������ޤޤ�while��Ƭ�� */
			continue;
		}
		ctl.unlock();

		/* DSP�ǥХ�����ͽ�󤷤��Ԥ� */
		dsp->waitLock(this);
		/* ���ϥǥХ����Υ����ץ���ǧ */
		dsp->open();

		char src[req.src.size() + 1];
		strcpy(src, req.src.c_str());
		strcpy(dbgstr, src);
		/* ȯ��®�٤�������*/
		req.iSpeed += iSpeedOffset;
		req.iSpeed /= iSpeedFactor;
		if (req.iSpeed > DT_MAX_SPEED) {
			req.iSpeed = DT_MAX_SPEED;
		}

		/* �ݥ��󥿤ν���� */
		pcBeg = src;
		iLen = strlen(src);

		/* ʸ���󤬽����ޤǥ롼�� */
		while(1) {
			/* ��Ƭ�Υ��ڡ�������� */
			while(*pcBeg == ' ' && *pcBeg != '\0')
				pcBeg++;
#if DEBUG >= 3
			printf("remaining: %s\n", pcBeg);
#endif
			/* pcBeg, pcEnd��Ʊ������ */
			pcEnd = pcBeg;

			/* ʸ����ν�λ�����å� */
			if (*pcBeg == '\0') {
				break;
			}

			if (*pcBeg == '[') {
				/* DECTalk��in-text-command����������ݥ��󥿤�ʤ�� */
				/* pcEnd�ǥ��ޥ�ɤν�����õ���Ƥ�����\0�򥻥å�*/
				while (*pcEnd != ']' && *pcEnd != '\0')
					pcEnd++;
				*pcEnd = '\0';
				/* ���ޥ�ɤ��� */
				dt_do_bracket_command(pcBeg+1);
				/* pcBeg��NULLʸ���ΤȤ���� */
				pcBeg = pcEnd;
				/*ʸ����κǸ�����å����롼��æ�� */
				if (pcBeg - src >= iLen)
					break;
				/* pcBeg��NULLʸ���μ��ذ�ư���ƥ롼�׺ǽ�� */
				pcBeg++;
				continue;
			}

			/* ���ޥ�ɤǤʤ�ʸ������ϰϤ���� */
			while (*pcEnd != '[' && *pcEnd != '\0'
						 && pcEnd - src < iLen) {
				if (iskanji(*pcEnd)) pcEnd+=2;
				else pcEnd++;
			}

			/* Ⱦ��/���Ѥαѿ������ϰϤ���� */
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
				/* �ѿ�����ν���꤫ʸ����ν����ʤ�Ѹ좪�����Ѵ� */
				if (engFlag != engFlagTmp || pcWork == pcEnd) {
					if (engFlag == TRUE) {
						if (jpEngToKana(&engKanaDic, pcWork2,
														(pcWork-pcWork2), &pcEngKanaResult)
								) {
#if DEBUG >= 3
							printf("converted: %s\n", pcEngKanaResult);
#endif
							/* �Ѵ���̤�append */
							tmpStr.append(pcEngKanaResult);
							/* jpEngToKana()��malloc()������������ */
							free (pcEngKanaResult);
						} else {
							/* �Ѵ��Ǥ��ʤ��ä��Ȥ����ѻ��δ֤˥��ڡ��������������ɲ� */
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
						/* �ѿ�����Ǥʤ������������ʤ��Ѵ��Ǥ��ʤ��ä��Ȥ�
							 ���Τޤ�append */
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
			} /* ʸ�����tmpstr������������λ */

			// tmpstr���Ф����ɤߤ��ִ�����
			replace_reading(tmpStr, req.punctuation);

			/* �������������� */
#if DEBUG >= 3
			printf("speaking: %s\n", tmpStr.c_str());
#endif
			pcBeg = pcEnd;

			/* �⡼������ */
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

			/* ������� */
			iRet = dt_lang_anal((char *)tmpStr.c_str());
			if (iRet != SS_LANG_ANAL_OK) {
				goto EXIT_FAIL;
			}
			/* �Ѵ���ɽ��ʸ���󤬶��ξ��ϥ롼����Ƭ�� */
			if (tLng.pszDst == NULL) {
				continue;
			}
	
			/* ���ߤβ���������ȷ�������ȿ�� */
			setVolume(vset->volume());
			setType(vset->type());
			setPitch(vset->pitch());
			setPitchRate(vset->pitchRate());
			setTone(vset->tone());
			/* ʸ���ɤߥ⡼�ɤ���ʸ���ʤ�ԥå���夲�롣 */
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
			 * ɽ��ʸ�����PCM�ǡ������Ѵ�����
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

			/* ������ɥǥХ����˽��� */
			for ( ; ; ) {
				/* check stop request */
				if (isStop()) {
					status = SS_STAT_NULL;
					goto STOP_OK;
				}
				/* ���ꥵ����ʬ��PCM�ǡ������� */
				iErrorInfo = SYT_generatePcm ( SynHandle, 
																			 pcPcmBuf,
																			 SS_PCMSIZE, 
																			 &dwGenSize );
				if ( iErrorInfo == SYRET_CONTINUE || iErrorInfo == SYRET_END )
					{
						/* �ְ��������򤷤ƥ�����ɥǥХ����� */
						if (iErrorInfo == SYRET_CONTINUE) {
							int size;
#if 1
							size = ssWarpPcm((short *)pcPcmBuf, dwGenSize/2, 0.95);
#else
							size = ssSkipPcm((short *)pcPcmBuf, dwGenSize/2, SS_SKIP_UNIT, SS_SKIP_FRAMES);
#endif
							/* PCM�ǡ����򥪡��ǥ����ǥХ����˽񤭹��� */
							if ( dsp->write ( pcPcmBuf,
																size*2 ) != size*2 )
								{
									fprintf (stderr, "DT: cannot write to audio\n");
								}
						}
						/* �⤦��������PCM�ǡ����ϻĤäƤ��ʤ� */
						if ( iErrorInfo == SYRET_END ) {
							memset(pcPcmBuf, 0, SS_PCMSIZE);
							dsp->write(pcPcmBuf, SS_PCMSIZE);
							break;
						}
					}
				else
					{
						/* PCM�ǡ����������˼��Ԥ��� */
						fprintf (stderr, "DT: SYT_generatePcm failed\n");
						goto EXIT_FAIL;
					}
			}
		}
#if DEBUG >= 3
		printf("output complete\n");
#endif
		/* �ꥯ�����ȼ����Τ����å� */
		ctl.lock();
		continue;

	STOP_OK:
		SYT_generatePcmStop(SynHandle);
#if DEBUG >= 3
		printf("stop request accepted\n");
#endif
		vset->selectVoice("paul");
		/* �ꥯ�����ȼ����Τ����å� */
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
	/* ���������ν���� */
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
	/* DSP�ǥХ����ν��֤�ͽ�� */
	dsp->reserveLock(this);
	ctl.lock();
	q.push((void *)req);
	ctl.unlock();
	ctl.cond_broadcast();
	return 0;
}

/* str����λ��ꤷ����������ɤߤ��֤������롣
 �ɤߤ�dt.h��SJIS��ʸ��������Ȥ����������Ƥ��롣
 mode��punctuation mode�ǡ�����ˤ�ä�ư��ۤʤ롣*/
int
dTalkParam::replace_reading(string& str, int mode)
{
	int matches = 0;
	size_t pos, match;
	char *char_to_replace = "@()\'\""; // �ִ������оݤ�ʸ���򤳤��ˤʤ�٤롣
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

/* thread���ϤΤ���Υ�åѡ��� */
void *
__dt_speak(void *arg)
{
	dTalkParam *ss = (dTalkParam *)arg;
	ss->dt_speak();
	return 0;
}
