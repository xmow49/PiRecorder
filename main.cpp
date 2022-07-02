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

#define OLED_WIDTH 128
#define OLED_HEIGHT 64

SSD1306 OLED(OLED_WIDTH, OLED_HEIGHT); // instantiate  an object 



string generateFilename() {

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char name[50];
    sprintf(name, "%02d-%02d-%d_%02d-%02d-%02d.wav", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
    return name;
}

bool recordState = false;

void record(string soundCards[]) {
        while (recordState) {
            string cmd = "arecord -D plughw:" + soundCards[0] + " --duration=7200 -r 48000 --format=S16_LE " + recordPath + generateFilename();
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
    for (int i = 0; i < (int)result.length(); i++) {
        if (result[i] == 10) {
            //new line
            int find = line.find("card");
            if (find != -1) {
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

void printTimeOLED(time_t recordStartTime, bool recordState) {
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
        OLED.setCursor(15, 25);
        OLED.print(text);

        if (recordState) {
            OLED.setTextSize(1);
            OLED.setCursor(0, 0);
            OLED.print("REC");
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


int main(int argc, char** argv) {
    signal(SIGINT, stopProgram);

    printf("Pi Recorder\n");

    string soundCards[10];
    getSoundCard(soundCards);

    setupOLED();
    
    uint8_t screenBuffer[OLED_WIDTH * (OLED_HEIGHT / 8) + 1];
    OLED.buffer = (uint8_t*)&screenBuffer;  // set that to library buffer pointer

    OLED.OLEDclearBuffer(); // Clear active buffer 
    OLED.setTextColor(WHITE);
    OLED.setCursor(0, 0);
    OLED.setTextSize(2);
    OLED.setCursor(55, 10);
    OLED.print("Pi");
    OLED.setCursor(20, 40);
    OLED.print("Recorder");
    OLED.OLEDupdate();  //write to active buffer

    usleep(2 * 1000 * 1000);

    std::thread recordThread;

    uint8_t bouton = 16; //BCM16 --> GPIO36

    bcm2835_gpio_fsel(bouton, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(bouton, BCM2835_GPIO_PUD_UP);

    bool boutonState, boutonStateOLD;

    boutonState = bcm2835_gpio_lev(bouton);
    boutonStateOLD = boutonState;

    time_t recordStartTime = 0;
    while (mainLoop) {
        boutonState = bcm2835_gpio_lev(bouton);
        if (boutonState != boutonStateOLD) {
            boutonStateOLD = boutonState;
            if (boutonState == 0) { //button push
                recordState = !recordState;
                if (recordState) {
                    printf("start Recording\n");
                    recordThread = std::thread(record, soundCards);
                    recordStartTime = std::time(nullptr);
                }
                else {
                    printf("stop Recording\n");
                    system("killall arecord");
                    recordThread.join();
                    recordStartTime = 0;
                }
            }
            usleep(50 * 1000);
        }
        printTimeOLED(recordStartTime, recordState);
        usleep(100 * 1000);
    }

    OLED.OLEDclearBuffer(); // Clear active buffer 
    OLED.OLEDupdate();  //write to active buffer
    recordThread.detach();
    OLED.OLEDPowerDown(); //Switch off display
    bcm2835_close(); // Close the library
    return 0;
}


