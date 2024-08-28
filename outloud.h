/* -*- mode: c++; tab-width: 2 -*- */
/*
** outloudfunc.h --- Voice synthesis functions.
**
** Copyright (C) 2001 Bilingual Emacspeak Project <seiken@argv.org>
**
** Last modified: $Date: 2002/05/05 17:35:28 $ by $Author: seiken $
** Keywords: Emacs, Emacspeak, speech, Linux
**
** This file is part of BEP (Bilingual Emacspeak Project) 
** <http://www.argv.org/bep/>
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

#ifndef outloud_h
#define outloud_h

#include <string>
#include <eci.h>
#include "ssdefs.h"
#include "dsp.h"
#include "tts.h"

class TTS_Outloud: public tts_engine
{
	int status;
	thread_ctl ctl;
	pthread_t th;
	queue q;
	int q_lock;
	int Punc_Mode;
	ECIHand hECI; // Viavoice ECI handle
	ssDspDevice *dsp;		/* 音声出力デバイス */
	int Outloud_init();
	int Outloud_set_default();
	int Outloud_close();
	int ChkError();
	int set_speed(int iSpeed);
	bool Outloud_Translate(ssRequest *req);
	int set_pitch(int pitch);
	int replace_reading(string &, int);
	
public:
	TTS_Outloud(void);
	~TTS_Outloud(void);
	void setDsp(ssDspDevice *);
	int start();
	int shutdown();
	int add(ssRequest *);
	int speak();
	void setStatus(int);
	int getStatus();
	void Outloud_Speak();
	int isStop() { return getStatus() == SS_STAT_STOP_REQUESTED; }
	ssDspDevice *getDsp() { return dsp;}
};


// ECI Command string
/*
struct ol_voice {
	const char name[];
	const char cmd[];
};
*/

const char DefPitch[] = "`v1 ";
const char UpPitch[] = " `vb150 ";
const char paul[] = " `v1 ";
const char harry[] = " `v1 `vh65 `vb240 ";
const char dennis[] = " `v1  `vb60 ";
const char frank[] = " `v1 `vr100 ";
const char betty[] = " `v7 `vv60000 `vb280";
const char ursula[] = " `v2 ";
const char rita[] = " `v2 `vr100 ";
const char wendy[] = " `v2 `vy50 ";
const char kit[] = " `v3 ";
const char paul_monotone[] = " `v1 `vf0 `vb90 `vv58000 ";
const char paul_bold[] = "`v1 `vr10 `vf75 `vh60 `vb200 `vv60000 ";
const char paul_italic[] = " `v1 `vh37 `vb300 `vf100 `vv60000 ";
const char paul_smooth[] = " `v1 `vr0 `vh40 `vb260 `vf75 `vv55000 ";
const char anotation_voice[] = " `v1 `vr0 `vh40 `vb260 `vf75 `vv55000 ";
const char indent_voice[] = " `v1 `vr80 `vb80 `vh45 `vf35 `p2 `vv443000 ";
const char paul_animated[] = " `v1 `vf65 `vh45 `vb180 `vy0 `vv60000 ";

#endif
