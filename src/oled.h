#include <string>
#include <time.h>
#include <vector>

using namespace std;
	
void setupOLED();
void shutdownOLED();

void printTimeOLED(time_t recordStartTime, bool recordState, string filename);
void printInfo(string updateIP = "", long long updateUsed = 0, long long updateSize = 0);
string humanReadable(long long size);

void printSoundCardError();
void printSoundCardRetry();

void printMenuTitle(char* text);

unsigned int printPlayList(std::vector<string> newFiles, unsigned char action);
void printPlay(std::vector<string> files, string recordsPath, unsigned int index);

void updatePlayingDisplay(uint32_t currentFrame, uint32_t totalFrame, uint32_t sampleRate, bool playingState);