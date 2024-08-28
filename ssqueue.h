/* -*- Mode: c++; tab-width: 2 -*- */
/*
** ssqueue.h --- implementation of speech queue(header).
**
** Copyright (C) 2000, 2001 Bilingual Emacspeak Project <inoue@argv.org>
**
** Last modified: $Date: 2002/01/27 06:27:56 $ by $Author: seiken $
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
#ifndef SS_QUEUE_H
#define SS_QUEUE_H

#include "control.h"
#include "queue.h"
#include "tts.h"
#if defined(SS_DTALKER)
#include "dt.h"
#endif /* SS_DTALKER */

struct ss_queue_block {
	ssRequest *request;
	ss_queue_block *next;
	ss_queue_block(ssRequest *);
};

class ss_queue {
	thread_ctl ctl;
	ss_queue_block *top;
	ss_queue_block *end;
	int empty_internal();
public:
	ss_queue();
	~ss_queue();
	ss_queue_block *push_text(
					const char *text,
					int len, int type, int lang,
					int speed,
					int allcaps_beep,
					int capitalize,
					int split_caps,
					int punctuation);
	ss_queue_block *push_text_add(
				const char *text,
				int len);
	ssRequest *pop();
	int empty();
	void clear();
	ssRequest *gethead();
	ssRequest *gettail();
};

#endif /* SS_QUEUE_H */

