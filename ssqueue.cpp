/*
** ssqueue.cpp --- implementation of speech queue.
**
** Copyright (C) 2000, 2001 Bilingual Emacspeak Project <inoue@argv.org>
**
** Last modified: $Date: 2002/02/11 16:41:01 $ by $Author: inoue $
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
#include <pthread.h>
#include "ssdefs.h"
// #include "dt.h"
#include "ssqueue.h"

ss_queue_block::ss_queue_block(ssRequest *req) {
  request = req;
  next = NULL;
}

ss_queue::ss_queue()
{
  top = NULL;
  end = NULL;
  ctl.init();
}

ss_queue::~ss_queue()
{
  clear();
  ctl.destroy();
}

/* 要素を挿入する。 */
ss_queue_block *
ss_queue::push_text(
		    const char *text,
		    int len,
		    int type,
		    int lang,
		    int speed,
		    int allcaps_beep,
		    int capitalize,
		    int split_caps,
		    int punctuation)
{
  ss_queue_block *qb = NULL;
  ctl.lock();
  ssRequest *req = new ssRequest();
  req->src.append(text, len);
  req->type = type;
  req->lang = lang;
  req->iSpeed = speed;
  req->allcaps_beep = allcaps_beep;
  req->capitalize = capitalize;
  req->punctuation = punctuation;
  req->split_caps = split_caps;
  if (top == NULL) {
    top = new ss_queue_block(req);
    end = top;
  } else {
    qb = new ss_queue_block(req);
    end->next = qb;
    end = qb;
  }
  end->next = NULL;
  ctl.unlock();
  return qb;
}

/* queueの最後の要素のテキストに追加する */
ss_queue_block *
ss_queue::push_text_add(const char *text, int len)
{
  ctl.lock();
  if (end == NULL) {
    ctl.unlock();
    return NULL;
  }
  end->request->src.append(text, len);
  ctl.unlock();
  return end;
}

/* 先頭要素をpopする */
ssRequest *
ss_queue::pop()
{
  ss_queue_block *qb;
  ssRequest *req;
  ctl.lock();
  if (top == NULL) {
    ctl.unlock();
    return NULL;
  }
  qb = top;
  req = qb->request;
  top = qb->next;
  delete qb;
  if (top == NULL) {
    end = NULL;
  }
  ctl.unlock();
  return req;
}

/* queueが空かどうか返す */
int
ss_queue::empty()
{
  ctl.lock();
  if (top == NULL) {
    ctl.unlock();
    return 1;
  }
  ctl.unlock();
  return 0;
}

/* クラス内利用のためのメソッド。ロックせずにemptyかどうか調べる。 */
int
ss_queue::empty_internal()
{
  return top == NULL;
}

/* queueをクリアする。 */
void
ss_queue::clear()
{
  ss_queue_block *qb;
  ctl.lock();
  while (!empty_internal()) {
    qb = top;
    top = qb->next;
    delete qb->request;
    delete qb;
    if (top == qb) {
      top = end = NULL;
      break;
    }
  }
  ctl.unlock();
  return;
}

/* 最初の要素に含まれるリクエストのポインタを返す。 */
ssRequest *
ss_queue::gethead()
{
  ssRequest *req;
  ctl.lock();
  if (top != NULL) {
    req = top->request;
  } else {
    req = NULL;
  }
  ctl.unlock();
  return req;
}

/* 最後の要素に含まれるリクエストのポインタを返す */
ssRequest *
ss_queue::gettail()
{
  ssRequest *req;
  ctl.lock();
  if (end != NULL) {
    req = end->request;
  } else {
    req = NULL;
  }
  ctl.unlock();
  return req;
}

