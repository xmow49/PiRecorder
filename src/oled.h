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

void printPlay(std::vector<string> newFiles, unsigned char action);