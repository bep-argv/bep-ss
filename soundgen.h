#ifndef SOUNDGEN_H
#define SOUNDGEN_H

class soundGenerator {
  float wave_x;
  int wave_samp;
  size_t buff_sz;
  int vol;
  int samp;
 public:
  soundGenerator(int samples, int volume, int buff_size)
    {
      samp = samples;
      vol = volume;
      wave_x = 0;
      wave_samp = 0;
      buff_sz = buff_size;
    }

  void setVolume(int volume) { vol = volume; }
  int getVolume() { return vol; }
  void init() { wave_x = wave_samp = 0; }
  void setBufferSize(int buff_size) { buff_sz = buff_size; }
  int getBufferSize() { return buff_sz; }
  void setSamples(int samples) { samp = samples; }
  int getSamples() { return samp; }
  int generate(int freq, int msec, char *buff);
};

#endif
