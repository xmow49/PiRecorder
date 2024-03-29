#include "alsa.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <bcm2835.h>
#include <thread>

// Include the ALSA .H file that defines ALSA functions/data
#include <alsa/asoundlib.h>
#include <alsa/control.h>

#include "config.h"
#include "gpio.h"
#include "oled.h"

using namespace std;

#pragma pack(1)
/////////////////////// WAVE File Stuff /////////////////////
// An IFF file header looks like this
typedef struct _FILE_head
{
	unsigned char ID[4];   // could be {'R', 'I', 'F', 'F'} or {'F', 'O', 'R', 'M'}
	unsigned int Length;   // Length of subsequent file (including remainder of header). This is in
						   // Intel reverse byte order if RIFF, Motorola format if FORM.
	unsigned char Type[4]; // {'W', 'A', 'V', 'E'} or {'A', 'I', 'F', 'F'}
} FILE_head;

// An IFF chunk header looks like this
typedef struct _CHUNK_head
{
	unsigned char ID[4]; // 4 ascii chars that is the chunk ID
	unsigned int Length; // Length of subsequent data within this chunk. This is in Intel reverse byte
						 // order if RIFF, Motorola format if FORM. Note: this doesn't include any
						 // extra byte needed to pad the chunk out to an even size.
} CHUNK_head;

// WAVE fmt chunk
typedef struct _FORMAT
{
	short wFormatTag;
	unsigned short wChannels;
	unsigned int dwSamplesPerSec;
	unsigned int dwAvgBytesPerSec;
	unsigned short wBlockAlign;
	unsigned short wBitsPerSample;
	// Note: there may be additional fields here, depending upon wFormatTag
} FORMAT;
#pragma pack()

// Size of the audio card hardware buffer. Here we want it
// set to 1024 16-bit sample points. This is relatively
// small in order to minimize latency. If you have trouble
// with underruns, you may need to increase this, and PERIODSIZE
// (trading off lower latency for more stability)
//#define BUFFERSIZE	(2*1024)
#define BUFFERSIZE (2 * 16)

// How many sample points the ALSA card plays before it calls
// our callback to fill some more of the audio card's hardware
// buffer. Here we want ALSA to call our callback after every
// 64 sample points have been played
#define PERIODSIZE (2 * 4)

// Handle to ALSA (audio card's) playback port
snd_pcm_t *PlaybackHandle;

snd_pcm_hw_params_t *params;

// Handle to our callback thread
snd_async_handler_t *CallbackHandle;

// Points to loaded WAVE file's data
unsigned char *WavePtr;

// Size (in frames) of loaded WAVE file's data
snd_pcm_uframes_t WaveSize;

snd_pcm_uframes_t paramFrames;

// Sample rate
unsigned short WaveRate;

// Bit resolution
unsigned char WaveBits;

// Number of channels in the wave file
unsigned char WaveChannels;

// The name of the ALSA port we output to. In this case, we're
// directly writing to hardware card 0,0 (ie, first set of audio
// outputs on the first audio card)
// static const char		SoundCardPortName[] = "plughw:1,0";
static const char SoundCardPortName[] = "plughw:1,0";

// For WAVE file loading
static const unsigned char Riff[4] = {'R', 'I', 'F', 'F'};
static const unsigned char Wave[4] = {'W', 'A', 'V', 'E'};
static const unsigned char Fmt[4] = {'f', 'm', 't', ' '};
static const unsigned char Data[4] = {'d', 'a', 't', 'a'};

/********************** compareID() *********************
 * Compares the passed ID str (ie, a ptr to 4 Ascii
 * bytes) with the ID at the passed ptr. Returns TRUE if
 *
 * a match, FALSE if not.
 */

unsigned char compareID(const unsigned char *id, unsigned char *ptr)
{
	register unsigned char i = 4;

	while (i--)
	{
		if (*(id)++ != *(ptr)++)
			return (0);
	}
	return (1);
}

/********************** waveLoad() *********************
 * Loads a WAVE file.
 *
 * fn =			Filename to load.
 *
 * RETURNS: 0 if success, non-zero if not.
 *
 * NOTE: Sets the global "WavePtr" to an allocated buffer
 * containing the wave data, and "WaveSize" to the size
 * in sample points.
 */

unsigned char waveLoad(const char *fn)
{
	const char *message;
	FILE_head head;
	register int inHandle;

	if ((inHandle = open(fn, O_RDONLY)) == -1)
		message = "didn't open";

	// Read in IFF File header
	else
	{
		if (read(inHandle, &head, sizeof(FILE_head)) == sizeof(FILE_head))
		{
			// Is it a RIFF and WAVE?
			if (!compareID(&Riff[0], &head.ID[0]) || !compareID(&Wave[0], &head.Type[0]))
			{
				message = "is not a WAVE file";
				goto bad;
			}

			// Read in next chunk header
			int a = 0;
			while (read(inHandle, &head, sizeof(CHUNK_head)) == sizeof(CHUNK_head))
			{
				a++;
				cout << "read " << a << endl;
				// ============================ Is it a fmt chunk? ===============================
				if (compareID(&Fmt[0], &head.ID[0]))
				{
					FORMAT format;

					// Read in the remainder of chunk
					if (read(inHandle, &format.wFormatTag, sizeof(FORMAT)) != sizeof(FORMAT))
						break;

					// Can't handle compressed WAVE files
					if (format.wFormatTag != 1)
					{
						message = "compressed WAVE not supported";
						goto bad;
					}

					WaveBits = (unsigned char)format.wBitsPerSample;
					WaveRate = (unsigned short)format.dwSamplesPerSec;
					WaveChannels = format.wChannels;
				}

				// ============================ Is it a data chunk? ===============================
				else if (compareID(&Data[0], &head.ID[0]))
				{
					// Size of wave data is head.Length. Allocate a buffer and read in the wave data
					cout << "mallocSize: " << head.Length << endl;
					if (!(WavePtr = (unsigned char *)malloc(head.Length)))
					{
						message = "won't fit in RAM";
						goto bad;
					}

					if (read(inHandle, WavePtr, head.Length) != (int)head.Length)
					{
						free(WavePtr);
						break;
					}

					// cout << "wavLoaded " << endl;
					// for (int i = 0; i < head.Length; i++) {
					//	cout << (int)WavePtr[i] << " ";
					//	if(i % 16 == 15) cout << endl;
					// }

					// Store size (in frames)
					WaveSize = (head.Length * 8) / ((unsigned int)WaveBits * (unsigned int)WaveChannels);

					close(inHandle);
					return (0);
				}

				// ============================ Skip this chunk ===============================
				else
				{
					if (head.Length & 1)
						++head.Length; // If odd, round it up to account for pad byte
					lseek(inHandle, head.Length, SEEK_CUR);
				}
			}
		}

		message = "is a bad WAVE file";
	bad:
		close(inHandle);
	}

	printf("%s %s\n", fn, message);
	return (1);
}

/********************** play_audio() **********************
 * Plays the loaded waveform.
 *
 * NOTE: ALSA sound card's handle must be in the global
 * "PlaybackHandle". A pointer to the wave data must be in
 * the global "WavePtr", and its size of "WaveSize".
 */

bool stopPlaying = false;


void play_audio()
{
	register snd_pcm_uframes_t count, frames;
	// Output the wave data
	count = 0;
	bool playState = true;

	do
	{
		int nFrame = 1000; // n frame will be play for this loop turn
		if ((int)WaveSize - (int)count < nFrame)
		{ // if the rest of the wave is less than nFrame, then play the rest of the wave
			nFrame = (int)WaveSize - (int)count;
		}
		frames = snd_pcm_writei(PlaybackHandle, WavePtr + 2 * count, nFrame); // play nFrame of the Wave
		// If an error, try to recover from it
		if (frames < 0)
			frames = snd_pcm_recover(PlaybackHandle, frames, 0);
		if (frames < 0)
		{
			printf("Error playing wave: %s\n", snd_strerror(frames));
			break;
		}
		// Update our pointer
		count += frames;
		// cout << "count: " << count << endl;

		bool state[3];

		readButtonsStates(buttons, state);

		if (state[B_OK] == false)
		{
			playState = false;
			updatePlayingDisplay(count, WaveSize, WaveRate, playState, true);
			while (state[B_OK] == false)
			{
				readButtonsStates(buttons, state);
			}
		}
		else if (state[B_RIGHT] == false)
		{
			count += 5 * WaveRate; // add 5 seconds to the count
			while (state[B_RIGHT] == false)
			{
				readButtonsStates(buttons, state);
			}
		}
		else if (state[B_LEFT] == false)
		{
			count -= 5 * WaveRate; // sub 5 seconds to the count
			while (state[B_LEFT] == false)
			{
				readButtonsStates(buttons, state);
			}
		}
		while (playState == false)
		{
			readButtonsStates(buttons, state);
			if (state[B_OK] == false)
			{
				playState = true;
				while (state[B_OK] == false)
				{
					readButtonsStates(buttons, state);
				}
				updatePlayingDisplay(count, WaveSize, WaveRate, playState, true);
			}
		}
		updatePlayingDisplay(count, WaveSize, WaveRate, playState, false);
	} while ((count < WaveSize) && !stopPlaying);
	// Wait for playback to completely finish
	if (count == WaveSize)
		snd_pcm_drain(PlaybackHandle);

	playState = false;
	updatePlayingDisplay(WaveSize, WaveSize, WaveRate, true, true);
}

/*********************** free_wave_data() *********************
 * Frees any wave data we loaded.
 *
 * NOTE: A pointer to the wave data be in the global
 * "WavePtr".
 */

void free_wave_data(void)
{
	if (WavePtr)
		free(WavePtr);
	WavePtr = 0;
}

void alsa_play(char path[])
{

	// system("amixer set Headphone unmuted 50%");

	// No wave data loaded yet
	WavePtr = 0;

	// Load the wave file
	if (!waveLoad(path))
	{
		register int err;

		// Open audio card we wish to use for playback
		if ((err = snd_pcm_open(&PlaybackHandle, &SoundCardPortName[0], SND_PCM_STREAM_PLAYBACK, 0)) < 0)
			printf("Can't open audio %s: %s\n", &SoundCardPortName[0], snd_strerror(err));
		else
		{
			switch (WaveBits)
			{
			case 8:
				err = SND_PCM_FORMAT_U8;
				break;

			case 16:
				err = SND_PCM_FORMAT_S16;
				break;

			case 24:
				err = SND_PCM_FORMAT_S24;
				break;

			case 32:
				err = SND_PCM_FORMAT_S32;
				break;
			}

			// Set the audio card's hardware parameters (sample rate, bit resolution, etc)
			//			snd_pcm_hw_params(PlaybackHandle, )
			if ((err = snd_pcm_set_params(PlaybackHandle, (snd_pcm_format_t)err, SND_PCM_ACCESS_RW_INTERLEAVED, WaveChannels, WaveRate, 1, 500000)) < 0)
				printf("Can't set sound parameters: %s\n", snd_strerror(err));

			// Play the waveform
			else
			{

				/*int dir;
				snd_pcm_hw_params_alloca(&params);
				snd_pcm_hw_params_any(PlaybackHandle, params);
				snd_pcm_hw_params_set_access(PlaybackHandle, params,
					SND_PCM_ACCESS_RW_INTERLEAVED);
				snd_pcm_hw_params_set_channels(PlaybackHandle, params, 2);

				paramFrames = 64;
				snd_pcm_hw_params_set_period_size_near(PlaybackHandle,
					params, &paramFrames, &dir);
				*/
				play_audio();
			}

			// Close sound card

			snd_pcm_close(PlaybackHandle);
		}
	}

	// Free the WAVE data
	free_wave_data();
}

thread playThread;

void createAudioThread(char *path)
{
	static char playingPath[100];
	strcpy(playingPath, path);
	stopPlaying = false;
	playThread = thread(alsa_play, playingPath);
}

void stopAudioThread(){
	stopPlaying = true;
	playThread.join();
}