/*
** ssqueue.cpp --- implementation of speech queue.
**
** Copyright (C) 2000, 2001 Bilingual Emacspeak Project <inoue@argv.org>
**
** Last modified: $Date: 2001/11/18 17:31:53 $ by $Author: inoue $
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
#include "ssdefs.h"
#include "queue.h"

queue_node::queue_node()
{
  content = NULL;
  next = NULL;
}

queue::queue()
{
  head = NULL;
  tail = NULL;
}

queue::~queue()
{
  clear();
}

queue_node *
queue::push(void *cnt)
{
  queue_node *q = new queue_node();
  q->content = cnt;
  q->next = NULL;
  if (tail != NULL) {
    tail->next = q;
  }
  tail = q;
  if (head == NULL) {
    head = q;
  }
  return q;
}

void *
queue::pop()
{
  void *p;
  queue_node *q;
  q = head;
  if (q != NULL) {
    head = q->next;
    if (head == NULL) tail = NULL;
    p = q->content;
    delete q;
    return p;
  }
  return NULL;
}

int
queue::empty()
{
  return (head == NULL);
}

void
queue::clear()
{
  queue_node *q, *q0;
  q = head;
  while(q != NULL) {
    q0 = q;
    q = q->next;
    delete q0->content;
    delete q0;
  }
  head = tail = NULL;
  return;
}

/* the same as clear() except that this function doesn't
   destroy content of each node. */
void
queue::clear_without_content()
{
  queue_node *q, *q0;
  q = head;
  while(q != NULL) {
    q0 = q;
    q = q->next;
    delete q0;
  }
  head = tail = NULL;
  return;
}

void *
queue::gethead()
{
  if (head == NULL) {
    return NULL;
  }
  return head->content;
}

void *
queue::gettail()
{
  if (tail == NULL) {
    return NULL;
  }
  return tail->content;
}
