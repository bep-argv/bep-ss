#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "config.h"
#include "ssdefs.h"
#include "control.h"
#include "dsp.h"
#if defined(SS_DTALKER)
#include "dt.h"
#endif /* SS_DTALKER */
#include "context.h"

ssContext::ssContext()
{
  char *dspName;
  ctl.init();
  status = SS_STAT_NULL;
  if ((dspName = getenv("BEP_SS_OUTPUT_DEVICE")) != NULL ) {
    dsp = new ssDspDevice(dspName);
  } else {
    dsp = new ssDspDevice(SS_SYN_OUTPUT_DEVICE);
  }
  tts_num = 0;
  working = 0;
  speed = SS_INIT_SPEED;
  punctuation = SS_PUNCTUATION_ALL;
  split_caps = FALSE;
  allcaps_beep = FALSE;
  capitalize = FALSE;
  language = SS_LANG_DEFAULT;
  char_scale = SS_INIT_CHAR_SCALE;
  sd = new soundGenerator(SS_DEFAULT_SAMPLES_PER_SEC, SS_TONE_VOLUME, SS_PCMSIZE);
}

ssContext::~ssContext()
{
  dsp->close();
  delete sd;
  delete dsp;
  ctl.destroy();
}

int
ssContext::stop(int sw)
{
  ctl.lock();
  if (sw != 0) {
    status = SS_STAT_STOP_REQUESTED;
    sq.clear();
    for (int i = 0; i < tts_num; i++) {
      tts[i]->setStatus(SS_STAT_STOP_REQUESTED);
    }
  } else {
    for (int i = 0; i < tts_num; i++) {
      tts[i]->setStatus(SS_STAT_NULL);
    }
  }
  status = SS_STAT_NULL;
  ctl.unlock();
  return sw;
}

int
ssContext::isStop()
{
  ctl.lock();
  int iRet = (status == SS_STAT_STOP_REQUESTED);
  ctl.unlock();
  return iRet;
}

int
ssContext::start()
{
  int ret = 0;
  for (int i = 0; i < tts_num; i++) {
    ret = tts[i]->start();
    if (ret != 0) {
      for (int j = i-1; j >= 0; j--)
	tts[j]->shutdown();
      return 1;
    }
  }
  return 0;
}

int
ssContext::shutdown()
{
  printf("ssContext: shutdown\n");
  for (int i=0; i < tts_num; i++) {
    tts[i]->shutdown();
  }
  return 0;
}

int
ssContext::register_tts(tts_engine *t, int lang)
{
  if (tts_num == SS_MAX_TTS) {
    return 0;
  }
  t->setDsp(dsp);
  t->setLanguage(lang);
  tts[tts_num++] = t;
  return tts_num;
}

int
ssContext::get_next_lang()
{
  ssRequest *req = (ssRequest *)(sq.gethead());
  if (req != NULL)
    return req->lang;
  return 0;
}

int
ssContext::get_lang_tts(int lang)
{
  int i;
  for(i = 0; i < tts_num && tts[i]->lang != lang; i++);
  if (i < tts_num) {
    return i;
  }
  return -1;
}

/* 言語の文字列名を数値IDに変換する。 */
int
ssContext::get_lang_id(const char *lname)
{
  static char *langNameIdTab[] = LANG_NAME_ID_TAB_INIT;
  int i;
  for (i=0; langNameIdTab[i] != NULL; i++) {
    if (strcmp(lname, langNameIdTab[i]) == 0)
      break;
  }
  if (langNameIdTab[i] == NULL)
    return 0;
  return i;
}

int
ssContext::set_language(const char *pcLang)
{
  int new_id = get_lang_id(pcLang);
  /* 言語を変更するのもそれを使ってssRequestをつくるのも同じスレッドなので、
     ロックはいらない。 */
  /* その言語のtTSがある時だけ変更を受け付ける。
     変更前のTTSが変更後の(実際の)入力をしゃべれるかどうかの問題は残る。 */
  if (get_lang_tts(new_id) != -1) {
    language = new_id;
#if DEBUG >= 2
    fprintf(stderr, "language set to %s\n", pcLang);
#endif
  } else {
#if DEBUG >= 2
    fprintf(stderr, "context: fallback: language is unchanged.\n");
#endif
  }
  return language;
}

/* ss_queueからデータを取り出してそれぞれのエンジンに割り振る。
 最後または言語の変わり目ではエンジンのspeak()メソッドを呼び出す。*/
int
ssContext::speak()
{
  ssRequest *req;
  int current_lang = 0; /* not initialized */
  int prev_lang = 0;
  unsigned int i;
  /* ss_queueが空になるまで繰り返す。 */
  while(!sq.empty()) {
    int new_lang = get_next_lang();
    if (current_lang != 0 && new_lang != current_lang) {
      /* 言語の変わり目 */
      int tts_id = get_lang_tts(current_lang);
      if (tts_id < 0)
	tts_id = 0;
      tts[tts_id]->speak();
      current_lang = new_lang;
      continue;
    }

    /* 最初の要素をpop */
    req = sq.pop();

    if (req->type == SS_WORK_TYPE_PLAIN
	|| req->type == SS_WORK_TYPE_PHONETIC
	|| req->type == SS_WORK_TYPE_LETTER) {
      /* スペースだけのリクエストは無視*/
      for (i = 0; i < req->src.length() && (req->src[i] == ' '); i++);
      if (i == req->src.length()) {
	/* 原語変わり目の判定もスキップしなければならないので、
	   current_langを直前の値に戻す。*/
	current_lang = prev_lang;
#ifdef WRITE_ELOG
	log_error(NULL, "dbg: skipping %d\n", current_lang);
#endif
	continue;
      }
      current_lang = req->lang;
      int tts_id = get_lang_tts(current_lang);
      if (tts_id < 0)
	tts_id = 0;
      // Process character scale
      if (req->type == SS_WORK_TYPE_LETTER)
	req->iSpeed = (int)(req->iSpeed * char_scale);
      // Send request to TTS
      tts[tts_id]->add(req);
      prev_lang = current_lang;
    } else if (req->type == SS_WORK_TYPE_TONE) {
      /* トーンをならす */
#if 1 /* this code is experimental and placing it here is bad idea. */
      play_tone(req->src.c_str());
#endif
      delete req;
    } else {
      /* 上記以外のtypeは無視 */
      delete req;
      continue;
    }
  }
  /* 最後に残った分を処理 */
  if (current_lang != 0) {
    int tts_id = get_lang_tts(current_lang);
    if (tts_id < 0)
      tts_id = 0;
    tts[tts_id]->speak();
  }
  return 0;
}

int
ssContext::play_tone(const char *cmd_args)
{
  /* 引数の文字列を周波数と長さの組に変換 */
  int fr, ms;
  char args[strlen(cmd_args) + 1];
  char *args2;
  strcpy(args, cmd_args);
  char *p = args;
  while (isdigit(*p))
    p++;
  *p = '\0';
  fr = atoi(args);
#if DEBUG >= 3
  fprintf(stderr, "fr: %d, ", fr);
#endif
  args2 = ++p;;
  while (!isdigit(*args2) && *args2 != '\0')
    args2++;
  if (*args2 == '\0')
    return 0;
  p = args2;
  while (isdigit(*p))
    p++;
  *p = '\0';
  ms = atoi(args2);
#if DEBUG >= 3
  fprintf(stderr, "ms: %d\n", ms);
#endif
  /* コマンドに従って音を出力 */
  int ret;
  char buf[SS_PCMSIZE];
  dsp->reserveLock(sd);
  dsp->waitLock(sd);
  dsp->open();
  do {
    ret = sd->generate(fr, ms, buf);
    dsp->write(buf, ret);
  } while(ret == SS_PCMSIZE);
  memset(buf, 0, SS_PCMSIZE);
  dsp->write(buf, SS_PCMSIZE);
  sd->init();
  dsp->releaseLock(sd);
  return 0;
}

int
ssContext::setRateOffset(char *lang, int offset)
{
  int lang_id = get_lang_id(lang);
  if (lang_id == 0)
    return 0;
  int tts_id = get_lang_tts(lang_id);
  if (tts_id < 0)
    tts_id = 0;
  tts[tts_id]->setRateOffset(offset);
  return offset;
}

