/* -*- Mode: c++; tab-width: 2 -*- */
/*
** outloudfunc.cpp --- Voice synthesis functions.
**
** Copyright (C) 2000, 2001 Bilingual Emacspeak Project <seiken@argv.org>
**
** Last modified: $Date: 2002/05/26 16:17:14 $ by $Author: inoue $
** Keywords: Emacs, Emacspeak, speech, Linux
**
** This file is part of BEP (Bilingual Emacspeak Project) 
** <http://www.argv.org/bep/>
** This file is originally written by Gary Bishop, and modified by BEP
** to make a bilingual, Japanese and English, DECTalk-like speech server 
** running under Linux.
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

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <pthread.h>
#include <string>
#include <time.h>
#include <eci.h>

#include "outloud.h"
#include "ssdefs.h"
#include "control.h"
#include "dsp.h"

// Static variable
static short PcmBuf[SS_PCMSIZE]; // PCM Buffer pointer

// Thread wrapper
void *
__ol_speak(void *arg)
{
	TTS_Outloud *ss = (TTS_Outloud *)arg;
	ss->Outloud_Speak();
	return 0;
}

// Callback function.
ECICallbackReturn callback
(ECIHand eciHand, ECIMessage msg, long lparam, void* data)
{
#if WRITE_ELOG && DEBUG >= 10
	log_error(NULL, "Outloud: callback function()\n");
#endif
	TTS_Outloud *ol = (TTS_Outloud *)data;
	int ret = eciDataProcessed;
	if (msg == eciWaveformBuffer) {
 	// if (!ol->isStop()) return ret;
 		ret = ol->getDsp()->write(PcmBuf, sizeof(short)*lparam);
 	}
#if WRITE_ELOG && DEBUG >= 10
	log_error(NULL, "Outloud: Callback finished %ld bytes PCM data\n", lparam);
#endif
	return eciDataProcessed;
	// return (ret?eciDataProcessed:eciDataNotProcessed);
}

// Class method
TTS_Outloud::TTS_Outloud(void)
{
	ctl.init();
	dsp = NULL;
	status = SS_STAT_NULL;
	lang = SS_LANG_EN;
	ctl.init();
}

TTS_Outloud::~TTS_Outloud(void)
{
	q.clear();
	ctl.destroy();
	return;
}

void
TTS_Outloud::setDsp(ssDspDevice *dspp)
{
	dsp = dspp;
}

void
TTS_Outloud::setStatus(int stat)
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
TTS_Outloud::getStatus()
{
	ctl.lock();
	int stat = status;
	ctl.unlock();
	return stat;
}

int TTS_Outloud::ChkError()
{
	int Err = eciProgStatus(hECI);
	char Msg[128];
	eciErrorMessage(hECI, Msg);
	printf("%x: %s\n", Err, Msg);
#ifdef WRITE_ELOG
	log_error(NULL, "%x: %s\n", Err, Msg);
#endif
	eciClearErrors(hECI);
	return Err;
}

int
TTS_Outloud::Outloud_init()
{
	iSpeedOffset = 0;
	hECI = eciNew();
	fprintf(stderr, "TTS_Outloud: IBM Viavoice outloud speech synth...\n");
	if (hECI == NULL_ECI_HAND) {
		fprintf (stderr, "Outloud: Create ECI handle failed\n");
		ChkError();
		return SS_OPEN_FAIL;
	}

	if (Outloud_set_default() == SS_OPEN_OK) {
		return SS_OPEN_OK;
	} else {
		return SS_OPEN_FAIL;
	}
}

/* set default parameters. */
int
TTS_Outloud::Outloud_set_default()
{
	// Setup calback function
	int iRet;
	eciRegisterCallback(hECI, &callback, (void *)this);
	if ((iRet = eciSetOutputBuffer(hECI, SS_PCMSIZE, PcmBuf)) == false) {
		printf("eciSetupBuffer() failed\n");
#ifdef WRITE_ELOG
		log_error(NULL, "eciSetupBuffer() failed\n");
#endif
		ChkError();
		return SS_OPEN_FAIL;
	}
	eciSetParam(hECI, eciSynthMode,1);
	eciSetParam(hECI, eciRealWorldUnits,1);
	eciSetParam(hECI,eciInputType,1);
	return SS_OPEN_OK;
}

int
TTS_Outloud::Outloud_close()
{
	eciDelete(hECI);
	return SS_CLOSE_OK;
}

int TTS_Outloud::set_speed(int iSpeed)
{
	eciSetVoiceParam(hECI, 0, eciSpeed, iSpeed);
	// eciSetVoiceParam(hECI, 0, eciSpeed, 80);
	return 0;
}

bool TTS_Outloud::Outloud_Translate(ssRequest *req)
{
#ifdef WRITE_ELOG
	log_error(NULL, "Str:\"%s\" Speed:%d Lang:%d, Type:%d\n",
						req->src.c_str(), req->iSpeed, req->lang, req->type);
#endif
	string spkstr = req->src;
	req->iSpeed += iSpeedOffset;
	Outloud_set_default();
	// erase begin space character
	while (spkstr.at(0) == ' ') {
		spkstr.erase(0, 1);
		if (spkstr.length() == 0) {
			return 0;
		}
	}
	set_speed(req->iSpeed);
	switch (req->type) {
	case SS_WORK_TYPE_PLAIN:
		// replace symbols
		// In-text-command 置き換え
		for(unsigned int beg=0, end=0; beg < spkstr.length(); beg++) {
			if (spkstr.at(beg) == '[') {
				// In-text command
				end = spkstr.find(']');
				// replace In-Text-Command
				if (memcmp(spkstr.c_str()+beg+1, ":np-monotone", 12) ==0) {
					spkstr.replace(beg, end-beg+1, paul_monotone);
					beg += sizeof(paul_monotone)-1;
					char Speed[8];
					snprintf(Speed, sizeof(Speed), "`vs%d " , req->iSpeed);
					spkstr.insert(beg, Speed, strlen(Speed));
				}
				else if (memcmp(spkstr.c_str()+beg+1, ":np-animated", 12) ==0) {
					spkstr.replace(beg, end-beg+1, paul_animated);
					beg += sizeof(paul_animated)-1;
					char Speed[8];
					snprintf(Speed, sizeof(Speed), "`vs%d ", req->iSpeed);
					spkstr.insert(beg, Speed, strlen(Speed));
				}
				else if (memcmp(spkstr.c_str()+beg+1, ":np-smooth", 9) ==0) {
					spkstr.replace(beg, end-beg+1, paul_smooth);
					beg += sizeof(paul_smooth)-1;
					char Speed[8];
					snprintf(Speed, sizeof(Speed), "`vs%d ", req->iSpeed);
					spkstr.insert(beg, Speed, strlen(Speed));
				}
				else if (memcmp(spkstr.c_str()+beg+1, ":np-indent", 9) ==0) {
					spkstr.replace(beg, end-beg+1, indent_voice);
					beg += sizeof(indent_voice)-1;
					char Speed[8];
					snprintf(Speed, sizeof(Speed), "`vs%d ", req->iSpeed);
					spkstr.insert(beg, Speed, strlen(Speed));
				}
				else if (memcmp(spkstr.c_str()+beg+1, ":np", 3) ==0) {
					spkstr.replace(beg, end-beg+1, paul);
					beg += sizeof(paul)-1;
					char Speed[8];
					snprintf(Speed, sizeof(Speed), "`vs%d ", req->iSpeed);
					spkstr.insert(beg, Speed, strlen(Speed));
				}
				else if (memcmp(spkstr.c_str()+beg+1, ":nb", 3) ==0) {
					spkstr.replace(beg, end-beg+1, betty);
					beg += sizeof(betty)-1;
					char Speed[8];
					snprintf(Speed, sizeof(Speed), "`vs%d ", req->iSpeed);
					spkstr.insert(beg, Speed, strlen(Speed));
				}
				else if (memcmp(spkstr.c_str()+beg+1, ":nh", 3) ==0) {
					spkstr.replace(beg, end-beg+1, harry);
					beg += sizeof(harry)-1;
					char Speed[8];
					snprintf(Speed, sizeof(Speed), "`vs%d ", req->iSpeed);
					spkstr.insert(beg, Speed, strlen(Speed));
				}
				else if (memcmp(spkstr.c_str()+beg+1, ":nu", 3) ==0) {
					spkstr.replace(beg, end-beg+1, ursula);
					beg += sizeof(ursula)-1;
					char Speed[8];
					snprintf(Speed, sizeof(Speed), "`vs%d ", req->iSpeed);
					spkstr.insert(beg, Speed, strlen(Speed));
				}
				else {
					spkstr.erase(beg, (end-beg+1));
				}
			}
		}
		replace_reading(spkstr, req->punctuation);
		break;
	case SS_WORK_TYPE_LETTER:
		if(isupper(spkstr.at(0))) {
			// 大文字はちょっと高い声で
			spkstr.insert(0, UpPitch, strlen(UpPitch));
			spkstr.insert(spkstr.length(), DefPitch, strlen(DefPitch));
		}
		break;
	default:
		break;
	}
	// Split caps.
	if(req->split_caps) {
		for (size_t idx=0; idx < spkstr.length(); idx++) {
			if(isupper(spkstr[idx]) && idx != 0) {
				spkstr.insert(idx++, " ", 1);
				// Fix some pronouciations
				if(spkstr[idx] == 'A') {
					spkstr.replace(idx, 1, "ey");
					idx++;
				}
			}
		}
	}
	printf("Speaking cmd: \"%s\"\n", spkstr.c_str());
	eciAddText(hECI, spkstr.c_str());
#ifdef WRITE_ELOG
	log_error(NULL, "Outloud: eciAddtext() \"%s\"\n", spkstr.c_str());
#endif
	/* DSPデバイスを予約して待つ */
	dsp->waitLock(this);
	/* 出力デバイスのオープンを確認 */
	dsp->open();
	if (!eciSynthesize(hECI)) {
#ifdef WRITE_ELOG
		log_error(NULL, "Outloud: eciSynthesize()\n");
#endif
		ChkError();
		eciClearInput(hECI);
#ifdef WRITE_ELOG
		log_error(NULL, "Outloud: eciClearInput()\n");
#endif
		return false;
	}
#ifdef WRITE_ELOG
	log_error(NULL, "Outloud: eciSynthesize()\n");
#endif
	while (eciSpeaking(hECI)) {
		if (isStop()) {
#ifdef WRITE_ELOG
			log_error(NULL, "Outloud: Stop command detected\n");
#endif
			// eciSynchronize(hECI);
			eciClearInput(hECI);
#ifdef WRITE_ELOG
			log_error(NULL, "Outloud: eciClearInput()\n");
#endif
			//eciReset(hECI);
			//Outloud_set_default();
			eciStop(hECI);
#ifdef WRITE_ELOG
			log_error(NULL, "eciStop()\n");
#endif
			status = SS_STAT_NULL;
#ifdef WRITE_ELOG
			log_error(NULL, "Outloud: Stop command accepted\n");
#endif
		}
	}
#ifdef WRITE_ELOG
	log_error(NULL, "Outloud: Translate() finished\n");
#endif
	return true;;
}

int
TTS_Outloud::replace_reading(string& str, int mode)
{
	int matches = 0;
	size_t pos, match;
	char *char_to_replace = "*-;()@\"`";
	char *char_to_pause = ".,!?;:+=/'\"@$%&_*()";
	if (mode == SS_PUNCTUATION_ALL) {
		// replace reading
		pos = match = 0;
		while(pos < str.length()) {
			match = str.find_first_of(char_to_replace, pos);
			if (match >= str.length())
				break;
			matches++;
			switch (str[match]) {
			case '*':
				str.replace(match, 1, " star ");
				pos = match + strlen(" star ");
				break;
			case '-':
				str.replace(match, 1, " dash ");
				pos = match + strlen(" dash ");
				break;
			case ';':
				str.replace(match, 1, " semicolon ");
				pos = match + strlen(" semicolon ");
				break;
			case '(':
				str.replace(match, 1, " left paren ");
				pos = match + strlen(" left paren ");
				break;
			case ')':
				str.replace(match, 1, " right paren ");
				pos = match + strlen(" right paren ");
				break;
			case '@':
				str.replace(match, 1, " at ");
				pos = match + strlen(" at ");
				break;
			case '\"':
				str.replace(match, 1, " quote ");
				pos = match + strlen(" quote ");
				break;
				/*
					case '`':
					str.replace(match, 1, " backqoute ");
					pos = match + strlen(" backquote ");
					break;
				*/
			default:
				pos = match + 1;
			}
		}

		// put pause
		pos = match = 0;
		char beforePause[] = " `00 ";
		char afterPause[] = " `p10 ";
		while(pos < str.length()) {
			match = str.find_first_of(char_to_pause, pos);
			if (match >= str.length())
				break;
			matches++;
			str.insert(match, beforePause);
			match += sizeof(beforePause); // length of char string +1
			str.insert(match, afterPause);
			match += sizeof(afterPause) - 1;
			pos = match;
		}
	} else {
		// punctuation = some, none
	}
	return matches;
}

void TTS_Outloud::Outloud_Speak()
{
	ssRequest req;
#if DEBUG >= 3
	fprintf(stderr, "Outloud: Creating thread\n");
#endif
	ctl.lock();
	/* threadが停止要求を受けるまでループ */
	while(ctl.isactive()) {

		/* キューにリクエストが入るまで待つ */
		while (q.empty()) {
			if (!ctl.isactive()) {
				break;
			}
#if DEBUG >= 3
			fprintf(stderr, "Outloud: waiting\n");
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
		printf("Outloud: get %s %d\n", request->src.c_str(), request->type);
		/* requestをコピーし、解放 .
			 これでメモリはスタックに取られるので、ループを抜けた段階で消滅する。*/
		req.dup(request);
		delete request;
		request = NULL;
#ifdef WRITE_ELOG
		log_error(NULL, "Outloud: get %s %d\n", req.src.c_str(), req.type);
#endif
		if (req.type == SS_WORK_TYPE_END) {
			/* DSPデバイスを次のエンジンに渡す */
			dsp->releaseLock(this);
			/* ctlをロックしたままでwhile先頭へ */
			continue;
		}
		ctl.unlock();

		Outloud_Translate(&req);
		ctl.lock();
	}
	ctl.unlock();
	return;
}

int
TTS_Outloud::add(ssRequest *req)
{
	ctl.lock();
	q.push((void *)req);
	ctl.unlock();
	return 0;
}

int
TTS_Outloud::speak()
{
	/* lock the DSP device */
	/* DSPデバイスの順番を予約 */
	dsp->reserveLock(this);
	ssRequest *req = new ssRequest();
	req->type = SS_WORK_TYPE_END;
	/* DSPデバイスの順番を予約 */
	ctl.lock();
	q.push((void *)req);
	ctl.unlock();
	printf("Outloud::speak() work_type_end\n");
	ctl.cond_broadcast();
	return 0;
}

int
TTS_Outloud::start()
{
	if(Outloud_init() == SS_OPEN_FAIL) {
		Outloud_close();
		return -1;
	}
	ctl.activate();
	int ret = pthread_create(&th, NULL, &__ol_speak, (void *)this);
	return ret;
}

int
TTS_Outloud::shutdown()
{
	printf("Outloud: shutdown\n");
	ctl.deactivate();
	ctl.cond_broadcast();
	pthread_join(th, NULL);
	Outloud_close();
	return 0;
}
