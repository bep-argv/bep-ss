#ifndef TTS_H
#define TTS_H

#include <string>
#include "dsp.h"
#include "ssdefs.h"
struct ssRequest {
	string src;
	int iLen;
	int type;
	int lang;
	int allcaps_beep;
	int capitalize;
	int punctuation;
	int split_caps;
	int ret;
	int iSpeed;		/* $BH/@<%9%T!<%I(B */
	ssRequest() {
		iSpeed = SS_INIT_SPEED;
		type = SS_WORK_TYPE_PLAIN;
		ret = 0;
	}
	int dup(ssRequest *s) {
		src = s->src;
		iLen = s->iLen;
		type = s->type;
		lang = s->lang;
		punctuation = s->punctuation;
		ret = s->ret;
		iSpeed = s->iSpeed;		/* $BH/@<%9%T!<%I(B */
		capitalize = s->capitalize;
		split_caps = s->split_caps;
		allcaps_beep = s->allcaps_beep;
		return 0;
	}
};

class tts_engine {
 protected:
	ssDspDevice *dsp;		/* $B2;@<=PNO%G%P%$%9(B */
	int iSpeedOffset; /* $BA4BN$NB.EY$K$3$l$rB-$9(B */
 public:
	int lang; /* $B%(%s%8%s$N8@8l(B(ssdefs.h) */
	virtual ~tts_engine() { return;}
	int setLanguage(int l) { return lang = l;}
	int setRateOffset(int offset) { return iSpeedOffset = offset;}
	virtual void setDsp(ssDspDevice *) = 0;
	virtual int start() = 0;
	virtual int shutdown() = 0;
	virtual int add(ssRequest *) = 0;
	virtual int speak() = 0;
	virtual void setStatus(int) = 0;
	virtual int getStatus() = 0;
};


#endif
