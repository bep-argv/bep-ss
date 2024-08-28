/*
** ssqueue.h --- implementation of speech queue(header).
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
#ifndef QUEUE_H
#define QUEUE_H

struct queue_node {
  void *content;
  queue_node *next;
  queue_node();
};

class queue {
  queue_node *head, *tail;
 public:
  queue();
  ~queue();
  queue_node *push(void *);
  void *pop();
  void *gethead();
  void *gettail();
  int empty();
  void clear();
  void clear_without_content();
};

#endif /* QUEUE_H */

