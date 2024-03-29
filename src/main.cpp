/*


for visual studio: sudo apt-get install openssh-server g++ gdb make ninja-build rsync zip libasound2-dev

wget http://www.airspayce.com/mikem/bcm2835/bcm2835-1.71.tar.gz
tar zxvf bcm2835-1.71.tar.gz
cd bcm2835-1.71/
./configure
make
sudo make check
sudo make install

curl -sL https://github.com/gavinlyonsrepo/SSD1306_OLED_RPI/archive/1.2.tar.gz | tar xz
cd SSD1306_OLED_RPI-1.2
sudo make

!! enable i2c in raspi-config


*/

#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <sstream>
#include <fstream>
#include <thread>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <stdexcept>
#include <bcm2835.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/statvfs.h>

#include "oled.h"
#include "config.h"
#include "alsa.h"
#include "gpio.h"
#include "files.h"

using namespace std;

string recordPath = "/mnt/records/";
string configPath = recordPath + ".recorder";

string getIPAddress()
{
	string ipAddress = "Unable to get IP Address";
	struct ifaddrs *interfaces = NULL;
	struct ifaddrs *temp_addr = NULL;
	int success = 0;
	// retrieve the current interfaces - returns 0 on success
	success = getifaddrs(&interfaces);
	if (success == 0)
	{
		// Loop through linked list of interfaces
		temp_addr = interfaces;
		while (temp_addr != NULL)
		{
			if (temp_addr->ifa_addr->sa_family == AF_INET)
			{
				// Check if interface is en0 which is the wifi connection on the iPhone
				if (strcmp(temp_addr->ifa_name, "en0"))
				{
					ipAddress = inet_ntoa(((struct sockaddr_in *)temp_addr->ifa_addr)->sin_addr);
				}
			}
			temp_addr = temp_addr->ifa_next;
		}
	}
	// Free memory
	freeifaddrs(interfaces);
	return ipAddress;
}

inline bool checkFileExist(const std::string &name)
{
	if (FILE *file = fopen(name.c_str(), "r"))
	{
		fclose(file);
		return true;
	}
	else
	{
		return false;
	}
}

string generateFilename()
{

	unsigned long fileId;

	if (checkFileExist(configPath))
	{
		cout << "config file exists" << endl;
		ifstream configFile(configPath);
		// read the file and split with =
		string line;
		while (getline(configFile, line))
		{
			stringstream ss(line);
			string key;
			string value;
			getline(ss, key, '=');
			getline(ss, value);

			if (key == "FILEID")
			{
				fileId = stoul(value);
			}
		}
		configFile.close();
	}
	else
	{
		cout << "config file does not exist" << endl;
		ofstream configFile;
		configFile.open(configPath);
		configFile << "FILEID=0" << endl;
		configFile.close();
		fileId = 0;
	}

	fileId++;
	cout << "New File ID: " << fileId << endl;

	ofstream configFile;
	configFile.open(configPath);
	configFile << "FILEID=" << fileId << endl;
	configFile.close();

	char name[15];
	sprintf(name, "R%06lu.wav", fileId);
	return name;
}

bool recordState = false;

void record(string soundCards[], string *filenameOUT)
{
	while (recordState)
	{
		string filename = generateFilename();
		*filenameOUT = filename;
		string cmd = "arecord -D plughw:" + soundCards[0] + " --duration=7200 -r 48000 --format=S16_LE " + recordPath + filename;
		system(cmd.c_str());
	}
}

std::string exec(const char *cmd)
{
	char buffer[128];
	std::string result = "";
	FILE *pipe = popen(cmd, "r");
	if (!pipe)
		throw std::runtime_error("popen() failed!");
	try
	{
		while (fgets(buffer, sizeof buffer, pipe) != NULL)
		{
			result += buffer;
		}
	}
	catch (...)
	{
		pclose(pipe);
		throw;
	}
	pclose(pipe);
	return result;
}

void getSoundCard(string strCardList[])
{
	string result = exec("arecord -l");
	string line;
	int cardIndex = 0;
	if (result.empty())
	{
		printf("No Sound Card\n");
		strCardList[0] = "-1";
		return;
	}
	for (int i = 0; i < (int)result.length(); i++)
	{
		if (result[i] == 10)
		{
			// new line
			if (line.find("card") != string::npos)
			{
				strCardList[cardIndex] = line[5]; // get the card id
				cardIndex++;
				printf("CARD: %c\n", line[5]);
			}
			line = "";
		}
		else
		{
			line += result[i];
		}
	}
}

void readSpace(string path, long long *used, long long *size)
{
	string result = exec("/bin/df");
	stringstream ss(result);
	string line;

	string cmdValues[6];
	int cmdIndex = 0;

	while (getline(ss, line))
	{ // for each line
		// cout << "Line: " << line << endl;
		stringstream key(line);
		string keyValue;
		cmdIndex = 0;
		cmdValues->clear();
		while (getline(key, keyValue, ' '))
		{ // for each info (split by spaces)
			if (keyValue != "")
			{ // If it is a value
				// cout << "Value: " << keyValue << endl;
				cmdValues[cmdIndex] = keyValue;
				cmdIndex++;
			}
		}
		if (cmdValues[5] == path.substr(0, path.size() - 1))
		{
			/*for (int i = 0; i < 6; i++) {
				cout << i << ": \"" << cmdValues[i] << "\"" << endl;
			}*/
			*used = stoll(cmdValues[2]) * 1000;
			*size = stoll(cmdValues[1]) * 1000;
		}
	}
}

bool mainLoop = true;
void stopProgram(int signal_number)
{
	recordState = false;
	system("killall arecord");
	mainLoop = false;
}

struct Display
{
	char menu = MENU_RECORD;
	bool active = true;
};

int main(int argc, char **argv)
{
	signal(SIGINT, stopProgram);
	signal(SIGTERM, stopProgram);
	printf("Pi Recorder\n");
	setupOLED();

	std::thread recordThread;

	for (uint i = 0; i < 3; i++)
	{
		bcm2835_gpio_fsel(buttons[i], BCM2835_GPIO_FSEL_INPT);
		bcm2835_gpio_set_pud(buttons[i], BCM2835_GPIO_PUD_UP);
	}

	string soundCards[10];
	getSoundCard(soundCards);

	while (soundCards[0] == "-1")
	{
		printSoundCardError();
		bool retry = false;
		while (!retry)
		{
			bool state[3];
			readButtonsStates(buttons, state);
			if (state[B_OK] == 0 || state[B_LEFT] == 0 || state[B_RIGHT] == 0)
			{ // button pressed
				printSoundCardRetry();
				getSoundCard(soundCards);
				retry = true;
				usleep(500 * 1000);
			}
		}
	}

	time_t recordStartTime = 0;

	bool buttonsStates[3] = {1, 1, 1};
	bool previousBoutonsStates[3] = {1, 1, 1};

	Display display;
	time_t startPushTime = time(nullptr);
	bool longPressRefresh = false;
	bool longPressStatus = false;

	while (mainLoop)
	{
		readButtonsStates(buttons, buttonsStates);
		static string currentFilename = "";

		for (uint i = 0; i < 3; i++)
		{
			if (i == B_OK && buttonsStates[B_OK] == 0 && time(nullptr) > startPushTime + 1 && !longPressRefresh && longPressStatus)
			{
				cout << "LONG PRESS END" << endl;
				longPressRefresh = true;
				longPressStatus = false;
				if (display.menu == MENU_PLAY)
				{
					stopAudioThread();
					display.menu = MENU_RECLIST;
				}
				display.active = !display.active;

				cout << "LongPress : active : " << display.active << endl;
			}
			if (buttonsStates[i] != previousBoutonsStates[i])
			{
				// new State
				if (buttonsStates[i] == 0)
				{ // when button pressed
					longPressStatus = true;
					startPushTime = std::time(nullptr);
				}
				else
				{ // action on release
				  // button pushed
					longPressStatus = false;
					switch (i)
					{
					case B_LEFT:
					{
						if (display.active)
						{
							switch (display.menu)
							{
							case MENU_INFO:
							{
								break;
							}
							case MENU_RECLIST:
							{
								std::vector<string> tmp;
								printPlayList(tmp, 'D');
								break;
							}
							default:
								break;
							}
						}
						else
						{
							display.menu = display.menu > 0 ? --display.menu : 0;
						}
						break;
					}
					case B_OK:
					{
						if (display.active)
						{
							switch (display.menu)
							{
							case MENU_RECORD:
							{
								recordState = !recordState;
								if (recordState)
								{
									printf("start Recording\n");
									recordThread = std::thread(record, soundCards, &currentFilename);
									recordStartTime = std::time(nullptr);
								}
								else
								{
									printf("stop Recording\n");
									system("killall arecord");
									recordThread.join();
									recordStartTime = 0;
								}
								break;
							}
							case MENU_INFO:
							{
								// string ip = "IP:" + getIPAddress();
								// long long used, size;
								// readSpace(recordPath, &used, &size);
								// cout << used << "/" << size << endl;
								// printInfo(ip, used, size);
								break;
							}
							case MENU_RECLIST:
							{
								display.menu = MENU_PLAY;
								vector<string> files = getFiles(recordPath);
								vector<string> tmp;
								unsigned int index = printPlayList(tmp, '\0'); // get selected file
								if (files.size() > 0)
								{
									printPlay(files, recordPath, index);
									string currentPlayingFile = recordPath + files[index];
									cout << currentPlayingFile << endl;
									int size = currentPlayingFile.length();
									char path[size + 1];
									strcpy(path, currentPlayingFile.c_str());
									createAudioThread(path);
								}

								break;
							}
							default:
								break;
							}
						}
						else
						{
							if (longPressRefresh)
								longPressRefresh = false;
							else
								display.active = !display.active;

							// on change menue
							if (display.active)
							{
								switch (display.menu)
								{
								case MENU_RECORD:
									break;
								case MENU_INFO:
								{
									string ip = "IP:" + getIPAddress();
									long long used, size;
									readSpace(recordPath, &used, &size);
									cout << used << "/" << size << endl;
									printInfo(ip, used, size);
									break;
								}
								case MENU_RECLIST:
								{
									vector<string> files = getFiles(recordPath);
									printPlayList(files, 1);
									break;
								}
								}
							}
						}

						break;
					}
					case B_RIGHT:
					{
						if (display.active)
						{
							switch (display.menu)
							{
							case MENU_INFO:
							{
								break;
							}
							case MENU_RECLIST:
							{
								std::vector<string> tmp;
								printPlayList(tmp, 'U');
								break;
							}
							default:
								break;
							}
						}
						else
						{
							display.menu = display.menu < 2 ? ++display.menu : 2;
						}

						break;
					}
					default:
						break;
					}
				}
				usleep(50 * 1000);
				previousBoutonsStates[i] = buttonsStates[i];
				cout << "Menu: " << +display.menu << " --> Active: " << display.active << endl;
			}
		}

		switch (display.menu)
		{
		case MENU_INFO:
		{
			if (display.active)
			{
				printInfo();
			}
			else
			{
				printMenuTitle("INFOS");
			}
			break;
		}
		case MENU_RECORD:
			if (display.active)
			{
				printTimeOLED(recordStartTime, recordState, currentFilename);
			}
			else
			{
				printMenuTitle("REC");
			}
			break;
		case MENU_RECLIST:
			if (display.active)
			{
			}
			else
			{
				printMenuTitle("ECOUTER");
			}
		case MENU_PLAY:
			if (display.active)
			{
				// printPlay("R000001.wav", "R000002.wav", "R000003.wav");
				// printPlay(files, recordPath, index);
			}
			else
			{
				printMenuTitle("ECOUTER");
			}
			break;
		default:
			break;
		}
		usleep(10 * 1000);
	}
	shutdownOLED();
	recordThread.detach();
	return 0;
}
