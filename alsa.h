static unsigned char compareID(const unsigned char* id, unsigned char* ptr);

static unsigned char waveLoad(const char* fn);

static void play_audio(void);

static void free_wave_data(void);
void alsa_play(char path[]);