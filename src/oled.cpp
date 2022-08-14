#include "oled.h"
#include "config.h"

#include "SSD1306_OLED.h"
#include <string>
#include <time.h>
#include <unistd.h>

#include <string>
#include <vector>
using namespace std;

SSD1306 OLED(OLED_WIDTH, OLED_HEIGHT); // instantiate  an object

void setupOLED()
{
	if (!bcm2835_init())
	{
		printf("Error 1201: init bcm2835 library\r\n");
	}

	OLED.OLEDbegin(); // initialize the OLED
	static uint8_t screenBuffer[OLED_WIDTH * (OLED_HEIGHT / 8) + 1];
	OLED.buffer = (uint8_t *)&screenBuffer; // set that to library buffer pointer

	OLED.OLEDclearBuffer(); // Clear active buffer
	OLED.setTextColor(WHITE);
	OLED.setTextSize(2);
	OLED.setCursor(5, 13);
	OLED.print("PiRecorder");
	OLED.OLEDupdate(); // write to active buffer
	usleep(1 * 1000 * 1000);
}

void shutdownOLED()
{
	OLED.OLEDclearBuffer(); // Clear active buffer
	OLED.OLEDupdate();		// write to active buffer
	OLED.OLEDPowerDown();	// Switch off display
	bcm2835_close();		// Close the library
}

void printTimeOLED(time_t recordStartTime, bool recordState, string filename)
{
	time_t now = time(nullptr);
	static time_t lastRefreshTime;

	if (now != lastRefreshTime)
	{ // every second
		lastRefreshTime = now;
		char text[15];
		if (recordStartTime == 0)
		{
			sprintf(text, "00:00:00");
		}
		else
		{
			time_t time = now - recordStartTime;

			long int hour = time / 60 / 60;
			long int minutes = time / 60;
			long int second = time - hour * 60 * 60 - minutes * 60;

			sprintf(text, "%02ld:%02ld:%02ld", hour, minutes, second);
		}
		OLED.OLEDclearBuffer(); // Clear active buffer
		OLED.setTextColor(WHITE);
		OLED.setTextSize(2);
		OLED.setCursor(15, 13);
		OLED.print(text);
		if (recordState)
		{
			OLED.setTextSize(1);
			OLED.setCursor(0, 0);
			OLED.print("REC");
			OLED.setCursor(30, 0);
			OLED.print(filename.c_str());
		}
		OLED.OLEDupdate();
	}
}

void printInfo(string updateIP, long long updateUsed, long long updateSize)
{
	static string ip = "";
	static long long used = 0;
	static long long size = 0;
	if (updateIP != "")
	{
		ip = updateIP;
	}
	if (updateUsed != 0)
	{
		used = updateUsed;
	}
	if (updateSize != 0)
	{
		size = updateSize;
	}

	OLED.OLEDclearBuffer();
	OLED.setCursor(0, 0);
	OLED.setTextSize(2);
	OLED.print("INFO");

	OLED.setTextSize(1);
	OLED.setCursor(0, 15);
	OLED.print(ip.c_str());

	OLED.setCursor(0, 24);
	char space[50];
	sprintf(space, "%s/%s", humanReadable(used).c_str(), humanReadable(size).c_str());
	OLED.print(space);
	OLED.OLEDupdate();
}

void printSoundCardError()
{
	OLED.OLEDclearBuffer();
	OLED.setTextSize(2);
	OLED.setCursor(20, 0);
	OLED.print("Error");
	OLED.setTextSize(1);
	OLED.setCursor(0, 15);
	OLED.print("No SoundCard deteced!");
	OLED.setCursor(0, 24);
	OLED.print("Press to retry");
	OLED.OLEDupdate();
}

void printSoundCardRetry()
{
	OLED.OLEDclearBuffer();
	OLED.setTextSize(2);
	OLED.setCursor(20, 13);
	OLED.print("Retry...");
	OLED.OLEDupdate();
}

string humanReadable(long long size)
{
	string suffixes[] = {"B", "KB", "MB", "GB", "TB"};
	int i = 0;
	while (size >= 1000)
	{
		size /= 1000;
		i++;
	}
	return to_string(size) + suffixes[i];
}

void printMenuTitle(char *text)
{
	OLED.OLEDclearBuffer();
	OLED.setTextSize(2);
	OLED.setCursor(20, 13);
	OLED.print(text);
	OLED.OLEDupdate();
}

void printPlay(std::vector<string> newFiles, unsigned char action)
{
	static std::vector<string> files;
	static unsigned int index = 1;
	if (newFiles.size() != 0)
		files = newFiles;

	static bool maxDOWN = 0;
	int toto = files.size();
	switch (action)
	{
	case 'U':

		if (maxDOWN)
			maxDOWN = false;
		else
			index = index < files.size() -2 ? ++index : index;
		break;
	case 'D':
		if (index == 0)
			maxDOWN = true;
		else
			index = index > 0 ? --index : index;
	default:
		break;
	}

	OLED.OLEDclearBuffer();
	OLED.setTextSize(1);
	OLED.setCursor(10, 0);
	OLED.print(files[index].c_str());

	if (maxDOWN)
	{
		OLED.setCursor(0, 0);
	}
	else
	{
		OLED.setCursor(0, 10);
	}

	OLED.print(">");

	OLED.setCursor(10, 10);
	OLED.print(files[index + 1].c_str());

	OLED.setCursor(10, 20);
	OLED.print(files[index + 2].c_str());

	OLED.OLEDupdate();
}