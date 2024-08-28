#ifndef DTVOICE_H
#define DTVOICE_H

#include "control.h"

struct DtVoice {
  string name;			/* name of voice */
  string init_string;		/* string to set modes of TTS engins */
  string type;
  int pitch;
  int pitch_rate;
  int tone;
  int volume;
};

struct DtVoiceSet {
  DtVoice *voices;
  int voiceNum;
  int currentVoice;
  int iSpeed;
  int iPunctMode;
  thread_ctl ctl;
  DtVoiceSet();
  ~DtVoiceSet();
  int selectVoice(const char *);
  string &initString();
  string &type();
  int pitch();
  int pitchRate();
  int tone();
  int volume();
};

#endif
