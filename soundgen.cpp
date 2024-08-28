#include <stdio.h>
#include <math.h>
#include "soundgen.h"

int
soundGenerator::generate(int freq, int msec, char *buff)
{
  int out_sz = 0;
  int buf_samp = buff_sz / 2;
  float frac = (float)freq / samp * M_PI * 2;
  int time_samp = (int)(samp * (msec / 1000.0));
  if (wave_samp > time_samp) {
    return -1;
  }
  short *buf_p = (short *)buff;
  int v = (int)(((float)vol / 100) * 32767);
  for (out_sz = 0; out_sz < buf_samp
	 && out_sz < (time_samp-wave_samp);
       out_sz++, wave_x += frac) {
    *(buf_p++) = (short)(sin(wave_x) * v);
  }
  wave_samp += out_sz;
  return out_sz * 2;
}

