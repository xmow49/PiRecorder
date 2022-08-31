unsigned char compareID(const unsigned char* id, unsigned char* ptr);

unsigned char waveLoad(const char* fn);

void play_audio(void);

void free_wave_data(void);
void alsa_play(char path[]);

void createAudioThread(char* path);

void stopAudioThread();