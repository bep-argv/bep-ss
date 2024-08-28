# Makefile# $Id: Makefile,v 1.11.2.22 2002/05/09 16:59:23 inoue Exp $
CC=g++
LD=g++
PROGRAM = bep-ss
PREFIX=/usr/local
INSTALL = /usr/bin/install -c
# For normal (optimized) compile
CPPFLAGS=-Wall -O2
# For debuggin compile
#CPPFLAGS=-Wall -ggdb -DDEBUG=3 -DWRITE_ELOG
#LDFLAGS=-ggdb
# Build with DTalker(Create System TTS)
OBJ_TTS_DTALKER = dtfunc.o dtvoice.o kanjifn.o dic.o 
HED_TTS_DTALKER = dic.h dt.h dtvoice.h kanjifn.h
LIB_DT = -lsyn -llng
# Build with IBM ViaVoice Outloud TTS
OBJ_TTS_OUTLOUD = outloudfunc.o
HED_TTS_OUTLOUD = outloud.h
LIB_VV = -libmeci50

############################################################
# DO NOT EDIT THE FOLLOWINGS
############################################################

H_ALL = cmdproc.h config.h context.h control.h dsp.h \
	$(HED_TTS_DTALKER) $(HED_TTS_OUTLOUD) queue.h soundgen.h spcsrv.h \
	ssdefs.h ssqueue.h tts.h 

OBJ_ALL = cmdproc.o context.o control.o dsp.o \
	queue.o soundgen.o ssfunc.o ssmain.o \
	ssqueue.o $(OBJ_TTS_DTALKER) $(OBJ_TTS_OUTLOUD)
all: $(PROGRAM)

$(PROGRAM): $(OBJ_ALL)
	$(LD) -ggdb -o $@ $^ -lm $(LIB_DT) $(LIB_VV) -lpthread
config.h: config.h.in
	cat config.h.in > config.h

$(OBJ_ALL): $(H_ALL)

install: $(PROGRAM)
	$(INSTALL) -m 555 $(PROGRAM) $(PREFIX)/bin
clean:
	rm -f *.o $(PROGRAM)
distclean:
	rm -f *.o $(PROGRAM) *~ config.h *core
