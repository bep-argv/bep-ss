#ifndef SSDEFS_H
#define SSDEFS_H

/* Version */
#define SS_VERSION "2.1"

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/* input buffer */
#define SS_INPUT_BLK_SIZE 512
/* PCM buffer size */
#define SS_PCMSIZE 2048
/* トーンジェネレータのボリューム(0--100) */
#define SS_TONE_VOLUME 30
/* initial speed */
#define SS_INIT_SPEED 300
#define SS_INIT_CHAR_SCALE 1.2

/* type of request queue */
#define SS_WORK_STR_MAX 16384
#define SS_WORK_TYPE_PLAIN 1  /* 入力は通常のテキスト */
#define SS_WORK_TYPE_PHONETIC 2  /* 入力はエンジン用フォネティック。言語処理不要 */
#define SS_WORK_TYPE_LETTER 3
#define SS_WORK_TYPE_END 4
#define SS_WORK_TYPE_TONE 5
#define SS_OPEN_OK 0
#define SS_OPEN_FAIL 1
#define SS_CLOSE_OK 0
#define SS_CLOSE_FAIL 1
#define SS_LANG_ANAL_OK 0
#define SS_LANG_ANAL_FAIL 1
#define SS_SPEAK_OK 0
#define SS_SPEAK_FAIL 1
#define SS_FLAG_UNSET 0
#define SS_FLAG_REQUEST_STOP 0x01
#define SS_PUNCTUATION_ALL 0
#define SS_PUNCTUATION_SOME 1
#define SS_PUNCTUATION_NONE 2
#define SS_LANG_JA 1
#define SS_LANG_EN 2
#define SS_LANG_DEFAULT SS_LANG_EN
#define SS_MAX_LANGNAME 12

/* ssDspDevice */
#define SS_DEFAULT_SAMPLES_PER_SEC 11050

/* ssContext */
#define SS_STAT_STOP_REQUESTED 1
#define SS_STAT_NULL 0
#define SS_MAX_TTS 10

/* ssfunc.cpp */
#define SS_SKIP_FRAME_SIZE 150
#define SS_SKIP_UNIT 4
#define SS_SKIP_FRAMES 1

/* Debug log function */
#ifdef WRITE_ELOG
#include <stdio.h>
int log_error(FILE * fpLog, char const* fmt, ...);
#endif // WRITE_LOG
#endif
