#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <fstream>
#include <thread>
#include <wiringPi.h>
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
    printf("now: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    int year = tm.tm_year + 1900;
    int month = tm.tm_mon + 1;
    int day = tm.tm_mday;
    int hour = tm.tm_hour;
    int minute = tm.tm_min;
    int seconds = tm.tm_sec;

    string result = to_string(year) + "-" + to_string(month) + "-" + to_string(day) + "--" + to_string(hour) + "-" + to_string(minute) + "-" + to_string(seconds);
    return result;
}


bool recordState = false;

void record(string soundCards[]) {
        while (recordState) {
            string cmd = "arecord -D plughw:" + soundCards[0] + " --duration=600 -r 48000 --format=S16_LE " + recordPath + generateFilename() + ".wav";
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
    for (int i = 0; i < result.length(); i++) {
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
    OLED.OLEDFillScreen(0xF0, 0); // splash screen bars

}


void printTimeOLED(time_t recordStartTime, bool recordState) {
    char text[15];
    if (recordStartTime == 0) {
        sprintf(text, "00:00:00");
    }
    else {
        time_t now = time(nullptr);
        time_t time = now - recordStartTime;

        uint8_t hour = time / 60 / 60;
        uint8_t minutes = time / 60;
        uint8_t second = time - hour * 60 * 60 - minutes * 60;

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




int main(int argc, char** argv) {
    printf("Pi Recorder\n");

    string soundCards[10];
    getSoundCard(soundCards);

    wiringPiSetup();
    setupOLED();
    

    uint8_t screenBuffer[OLED_WIDTH * (OLED_HEIGHT / 8) + 1];
    OLED.buffer = (uint8_t*)&screenBuffer;  // set that to library buffer pointer

    OLED.OLEDclearBuffer(); // Clear active buffer 
    OLED.setTextColor(WHITE);
    OLED.setCursor(0, 0);
    OLED.setTextSize(2);
    OLED.print("Pi Recorder");
    OLED.OLEDupdate();  //write to active buffer

    std::thread recordThread;

    int bouton = 27;

    pinMode(bouton, INPUT);
    pullUpDnControl(bouton,PUD_UP);

    bool boutonState, boutonStateOLD;

    boutonState = digitalRead(bouton);
    boutonStateOLD = boutonState;

    time_t recordStartTime = 0;
    while (1) {
        boutonState = digitalRead(bouton);
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
        usleep(50 * 1000);
    }

    recordThread.detach();
    OLED.OLEDPowerDown(); //Switch off display
    bcm2835_close(); // Close the library
    return 0;
}


