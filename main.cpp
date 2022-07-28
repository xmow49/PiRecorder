/*


for visual studio: sudo apt-get install openssh-server g++ gdb make ninja-build rsync zip

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
#include "SSD1306_OLED.h"

using namespace std;

string recordPath = "/mnt/records/";
string configPath = recordPath + ".recorder";

#define OLED_WIDTH 128
#define OLED_HEIGHT 32

#define B_LEFT 0
#define B_OK 1
#define B_RIGHT 2

#define MENU_INFO 0
#define MENU_RECORD 1
#define MENU_PLAY 2

SSD1306 OLED(OLED_WIDTH, OLED_HEIGHT); // instantiate  an object 

inline bool checkFileExist(const std::string& name) {
    if (FILE* file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    }
    else {
        return false;
    }
}

string generateFilename() {
    
    unsigned long fileId;

    if (checkFileExist(configPath)) {
		cout << "config file exists" << endl;
		ifstream configFile(configPath);
		//read the file and split with =
		string line;
		while (getline(configFile, line)) {
			stringstream ss(line);
			string key;
			string value;
			getline(ss, key, '=');
			getline(ss, value);

			if (key == "FILEID") {
				fileId = stoul(value);
			}
		}
		configFile.close();
    }
	else {
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
	
    char name[10];
    sprintf(name, "R%06lu.wav", fileId);
    return name;
}

bool recordState = false;

void record(string soundCards[], string* filenameOUT) {
        while (recordState) {
            string filename = generateFilename();
            *filenameOUT = filename;
            string cmd = "arecord -D plughw:" + soundCards[0] + " --duration=7200 -r 48000 --format=S16_LE " + recordPath + filename;
            system(cmd.c_str());
        }
}

std::string exec(const char* cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    }
    catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}

void getSoundCard(string strCardList[]) {
    string result = exec("arecord -l");
    string line;
	int cardIndex = 0;
    if (result.empty()) {
        printf("No Sound Card\n");
        strCardList[0] = "-1";
        return;
    }
    for (int i = 0; i < (int)result.length(); i++) {
        if (result[i] == 10) {
			//new line
            if (line.find("card") != -1) {
                strCardList[cardIndex] = line[5]; //get the card id
                cardIndex++;
                printf("CARD: %c\n", line[5]);
            }
            line = "";
        }
        else {
            line += result[i];
        }

    }
}

void setupOLED() {
    if (!bcm2835_init())
    {
        printf("Error 1201: init bcm2835 library\r\n");
    }

    OLED.OLEDbegin(); // initialize the OLED

}

void printTimeOLED(time_t recordStartTime, bool recordState, string filename) {
    time_t now = time(nullptr);
    static time_t lastRefreshTime;

    if (now != lastRefreshTime) { //every second
        lastRefreshTime = now;
        char text[15];
        if (recordStartTime == 0) {
            sprintf(text, "00:00:00");
        }
        else {
            time_t time = now - recordStartTime;

            long int hour = time / 60 / 60;
            long int minutes = time / 60;
            long int second = time - hour * 60 * 60 - minutes * 60;

            sprintf(text, "%02d:%02d:%02d", hour, minutes, second);
        }
        OLED.OLEDclearBuffer(); // Clear active buffer 
        OLED.setTextColor(WHITE);
        OLED.setTextSize(2);
        OLED.setCursor(15, 13);
        OLED.print(text);
        if (recordState) {
            OLED.setTextSize(1);
            OLED.setCursor(0, 0);
            OLED.print("REC");
            OLED.setCursor(30, 0);
            OLED.print(filename.c_str());
        }
        OLED.OLEDupdate();
    }
}

bool mainLoop = true;
void stopProgram(int signal_number) {
    recordState = false;
    system("killall arecord");
    mainLoop = false;
}


void readButtonsStates(const uint8_t buttonsPins[], bool buttonsStates[]) {
    for (uint i = 0; i < (sizeof(buttonsPins) / sizeof(*buttonsPins)); i++) {
        buttonsStates [i] = bcm2835_gpio_lev(buttonsPins[i]);
    }
}


int main(int argc, char** argv) {
    signal(SIGINT, stopProgram);
    signal(SIGTERM, stopProgram);
    printf("Pi Recorder\n");
    setupOLED();
    uint8_t screenBuffer[OLED_WIDTH * (OLED_HEIGHT / 8) + 1];
    OLED.buffer = (uint8_t*)&screenBuffer;  // set that to library buffer pointer

    OLED.OLEDclearBuffer(); // Clear active buffer 
    OLED.setTextColor(WHITE);
    OLED.setTextSize(2);
    OLED.setCursor(5, 13);
    OLED.print("PiRecorder");
    OLED.OLEDupdate();  //write to active buffer

    usleep(2 * 1000 * 1000);

    std::thread recordThread;

    uint8_t buttons[3] = { 26,27,28 };
    uint8_t bouton = 27; //BCM16 --> GPIO36

    for (uint i = 0; i < 3; i++) {
        bcm2835_gpio_fsel(buttons[i], BCM2835_GPIO_FSEL_INPT);
        bcm2835_gpio_set_pud(buttons[i], BCM2835_GPIO_PUD_UP);
        printf("%d\n", buttons[i]);
    }
    

    string soundCards[10];
    getSoundCard(soundCards);

    while (soundCards[0] == "-1") {        
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
        bool retry = false;
        while (!retry) {
            bool state[3];
            readButtonsStates(buttons, state);
            if (state[B_OK] == 0 || state[B_LEFT] == 0 || state[B_RIGHT] == 0) { //button pressed
                OLED.OLEDclearBuffer();
                OLED.setTextSize(2);
                OLED.setCursor(20, 13);
                OLED.print("Retry...");
                OLED.OLEDupdate();
                getSoundCard(soundCards);
                retry = true;
                usleep(500 * 1000);
            }
        }
    }


    time_t recordStartTime = 0;

    bool buttonsStates[3] = { 1 , 1 , 1 };
    bool previousBoutonsStates[3] = {1 , 1 , 1};

    uint8_t displayMenu = 1; 
    while (mainLoop) {
        readButtonsStates(buttons, buttonsStates);
        static string currentFilename = "";
        for (uint i = 0; i < 3; i++) {
            if (buttonsStates[i] != previousBoutonsStates[i]) {
                //new State
                if (buttonsStates[i] == 0) {
                    //button pushed

                    switch (i)
                    {
                    case B_LEFT:
                        displayMenu = displayMenu > 0 ? --displayMenu : displayMenu;
                        break;

                    case B_OK:
                        recordState = !recordState;
                        if (recordState) {
                            printf("start Recording\n");
                            recordThread = std::thread(record, soundCards, &currentFilename);
                            recordStartTime = std::time(nullptr);
                        }
                        else {
                            printf("stop Recording\n");
                            system("killall arecord");
                            recordThread.join();
                            recordStartTime = 0;
                        }
                        break;

                    case B_RIGHT:
                        displayMenu = displayMenu < 2 ? ++displayMenu : displayMenu;
                        break;
                    default:
                        break;
                    }
                    
                }
                usleep(50 * 1000);
                previousBoutonsStates[i] = buttonsStates[i];
            }
        }

        
        switch (displayMenu)
        {
        case MENU_INFO:
            OLED.OLEDclearBuffer();
            OLED.setCursor(15, 0);
            OLED.setTextSize(2);
            OLED.print("INFO");
            OLED.OLEDupdate();
            break;
        case MENU_RECORD:
            printTimeOLED(recordStartTime, recordState, currentFilename);
            break;
        case MENU_PLAY:
            OLED.OLEDclearBuffer();
            OLED.setCursor(15, 0);
            OLED.setTextSize(2);
            OLED.print("PLAY");
            OLED.OLEDupdate();
            break;
        default:
            break;
        }
        usleep(10 * 1000);
    }

    OLED.OLEDclearBuffer(); // Clear active buffer 
    OLED.OLEDupdate();  //write to active buffer
    recordThread.detach();
    OLED.OLEDPowerDown(); //Switch off display
    bcm2835_close(); // Close the library
    return 0;
}


