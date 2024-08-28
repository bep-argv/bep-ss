#include <string>
#include "ssdefs.h"
#include "dtvoice.h"

DtVoiceSet::DtVoiceSet()
{
  ctl.init();
  voiceNum = 10;
  voices = new DtVoice[voiceNum];

  voices[0].name = "paul";
  voices[0].init_string = "M3I2";
  voices[0].type = "male";
  voices[0].pitch = 3;
  voices[0].pitch_rate = 2;
  voices[0].tone = 0;
  voices[0].volume = 8;

  voices[1].name = "betty";				// use mary for betty
  voices[1].init_string = "F3I2";
  voices[1].type = "female";
  voices[1].pitch = 3;
  voices[1].pitch_rate = 2;
  voices[1].tone = 0;
  voices[1].volume = 8;

  voices[2].name = "paul-monotone";
  voices[2].init_string = "M3I0";
  voices[2].type = "male";
  voices[2].pitch = 3;
  voices[2].pitch_rate = 0;
  voices[2].tone = 0;
  voices[2].volume = 8;

  voices[3].name = "paul-animated";
  voices[3].init_string = "M4I3";
  voices[3].type = "male";
  voices[3].pitch = 4;
  voices[3].pitch_rate = 3;
  voices[3].tone = 0;
  voices[3].volume = 8;

  voices[4].name = "paul-smooth";
  voices[4].init_string = "M3I2";
  voices[4].type = "male";
  voices[4].pitch = 4;
  voices[4].pitch_rate = 1;
  voices[4].tone = 1;
  voices[4].volume = 9;

  voices[5].name = "indent-voice";
  voices[5].init_string = "M1I1";
  voices[5].type = "male";
  voices[5].pitch = 1;
  voices[5].pitch_rate = 1;
  voices[5].tone = 1;
  voices[5].volume = 7;

  voices[6].name = "harry";
  voices[6].init_string = "M0I2";
  voices[6].type = "male";
  voices[6].pitch = 1;
  voices[6].pitch_rate = 2;
  voices[6].tone = 0;
  voices[6].volume = 8;

  voices[7].name = "ursula";
  voices[7].init_string = "F2I2";
  voices[7].type = "female";
  voices[7].pitch = 1;
  voices[7].pitch_rate = 2;
  voices[7].tone = 0;
  voices[7].volume = 8;

  voices[8].name = "robosoft3";
  voices[8].init_string = "M3I1";
  voices[8].type = "male";
  voices[8].pitch = 3;
  voices[8].pitch_rate = 1;
  voices[8].tone = 1;
  voices[8].volume = 9;

  voices[9].name = "robosoft4";
  voices[9].init_string = "F3I1";
  voices[9].type = "female";
  voices[9].pitch = 3;
  voices[9].pitch_rate = 1;
  voices[9].tone = 0;
  voices[9].volume = 8;

  currentVoice = 0;
  iSpeed = SS_INIT_SPEED;
  iPunctMode = SS_PUNCTUATION_SOME;
}

DtVoiceSet::~DtVoiceSet()
{
  delete [] voices;
  ctl.destroy();
}

int
DtVoiceSet::selectVoice(const char *name)
{
  int i;
  ctl.lock();
  for (i=0; i < voiceNum; i++) {
    if (voices[i].name == name) {
      break;
    }
  }
  if (i == voiceNum) {
    ctl.unlock();
    return false;
  }
  else {
    currentVoice = i;
  }
  ctl.unlock();
  return true;
}

string &
DtVoiceSet::initString()
{
  ctl.lock();
  string *istr = &voices[currentVoice].init_string;
  ctl.unlock();
  return *istr;
}

string &
DtVoiceSet::type()
{
  ctl.lock();
  string *type = &voices[currentVoice].type;
  ctl.unlock();
  return *type;
}

int
DtVoiceSet::pitch()
{
  ctl.lock();
  int pitch = voices[currentVoice].pitch;
  ctl.unlock();
  return pitch;
}

int
DtVoiceSet::pitchRate()
{
  ctl.lock();
  int pitch_rate = voices[currentVoice].pitch_rate;
  ctl.unlock();
  return pitch_rate;
}

int
DtVoiceSet::tone()
{
  ctl.lock();
  int tone = voices[currentVoice].tone;
  ctl.unlock();
  return tone;
}

int
DtVoiceSet::volume()
{
  ctl.lock();
  int volume = voices[currentVoice].volume;
  ctl.unlock();
  return volume;
}
