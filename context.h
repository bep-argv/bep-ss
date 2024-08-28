#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include "dsp.h"
#include "soundgen.h"
#include "ssqueue.h"
#include "tts.h"

  /* If you change this array, you must change SS_LANG_* definitions
     in ssdefs.h */
#define LANG_NAME_ID_TAB_INIT {\
  "any", "ja", "en", NULL}

class ssContext {
  thread_ctl ctl;
  int speed;
  int punctuation;
  int split_caps;
  int allcaps_beep;
  int capitalize;
  float char_scale;
  int language;
  int working;
  int status;
  tts_engine *tts[SS_MAX_TTS];
  int tts_num;
  ssDspDevice *dsp;
  soundGenerator *sd;
  int get_next_lang();
  int get_lang_tts(int);
  int get_lang_id(const char *);
  int play_tone(const char *);
public:
  ss_queue sq;
  ssContext();
  ~ssContext();
  int stop(int);
  int isStop();
  int start();
  int shutdown();
  int register_tts(tts_engine *, int);
  int push_text(char *str, int len, int type) {
    sq.push_text(str, len, type,
		 language,
		 speed,
		 allcaps_beep,
		 capitalize,
		 split_caps,
		 punctuation);
    return 0;
  }
  int push_text_add(char *s, int len) {
    sq.push_text_add(s, len);
    return 0;
  }
  int speak();
  int tone(int, int);
  int set_speed(int sp) { return speed = sp; }
  int set_punctuation(int pm) { return punctuation = pm;}
  int set_split_caps(int sw) { return split_caps = sw;}
  int set_allcaps_beep(int sw) { return allcaps_beep = sw;}
  int set_capitalize(int sw) { return capitalize = sw;}
  int set_language(const char *);
  float set_char_scale(float scale) { return char_scale = scale;}
  int setRateOffset(char *lang, int offset);
};

#endif
