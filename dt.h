/* -*- mode: c++; tab-width: 2 -*- */
#ifndef DT_H
#define DT_H

#include <string>
#include <dtalker/synthe.h>
#include <dtalker/lgproc.h>
#include "control.h"
#include "queue.h"
#include "dic.h"
#include "dsp.h"
#include "dtvoice.h"
#include "tts.h"

#define DT_INIT_SPEED_FACTOR 30
#define DT_INIT_SPEED 9
#define DT_MAX_SPEED 9
#define DT_MAX_DIC_PATH_LEN 256

#define JA_AT_SJIS "\x83\x41\x83\x62\x83\x67\x83\x44"
#define JA_OPEN_PAREN_SJIS "\x83\x4a\x83\x62\x83\x52\x83\x71\x83\x89\x83\x4c"
#define JA_CLOSE_PAREN_SJIS "\x83\x4a\x83\x62\x83\x52\x83\x67\x83\x57"
#define JA_APOS_SJIS "\x83\x41\x83\x7c\x83\x58\x83\x67\x83\x8d\x83\x74\x83\x42"
#define JA_QUOTE_SJIS "\x83\x4e\x83\x49\x81\x5b\x83\x65\x81\x5b\x83\x56\x83\x87\x83\x93"

class dTalkParam: public tts_engine
{
	int status;
	thread_ctl ctl;
	pthread_t th;
	Dictionary engKanaDic;
	char *pszSynDic;						/* �ȷ�����ե�����̾ */
	char szLangDic[DT_MAX_DIC_PATH_LEN];			/* ���ܸ��켭��ե�����̾ */
	char szUsrDic1[DT_MAX_DIC_PATH_LEN];
	char szUsrDic2[DT_MAX_DIC_PATH_LEN];
	char szEngKanaDic[DT_MAX_DIC_PATH_LEN];
	int iSpeedFactor;	 /* ���Τ�®���ͤ򤳤�ǳ�롣 */
	SYNHANDLE SynHandle;				 /* �ȷ������饤�֥��ϥ�ɥ� */
	LNGHANDLE tLngHandle;				 /* ��������ϥ�ɥ� */
	LNGOPEN tLngOpen;		/* ������������ץ�¤�� */
	LNG tLng;				/* ���������¤�� */
	DtVoiceSet *vset;
	queue q;
	int q_lock;
	int dt_init();
	int dt_close();
	int dt_lang_anal(char *src);
	void dt_do_bracket_command(const char *cmd);
	int isStop() { return getStatus() == SS_STAT_STOP_REQUESTED; }
	int replace_reading(string& str, int mode);
public:
	dTalkParam();
	~dTalkParam();
	void setDsp(ssDspDevice *);
	int start();
	int shutdown();
	int add(ssRequest *);
	int speak();
	void setStatus(int);
	int getStatus();
	int setType(string);
	int setSpeed(int);
	int getSpeed();
	int setPitch(int);
	int getPitch();
	int setPitchRate(int);
	int setTone(int);
	int getTone();
	int setVolume(int);
	int getVolume();
	int setPunctuation(int);
	int getPunctuation();
	void *dt_speak();
};

#endif
