#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <fstream>
#include <thread>
#include <wiringPi.h>
#include <unistd.h>
using namespace std;

string recordPath = "/mnt/records/";

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

void record() {
        while (recordState) {
            string cmd = "arecord -D plughw:1 --duration=15 -r 48000 --format=S16_LE " + recordPath + generateFilename() + ".wav";
            system(cmd.c_str());
        }
}


int main(int argc, char** argv) {
    printf("Pi Recorder\n");

    wiringPiSetup();
    std::thread recordThread;

    int bouton = 27;

    pinMode(bouton, INPUT);
    pullUpDnControl(bouton,PUD_UP);

    bool boutonState, boutonStateOLD;

    boutonState = digitalRead(bouton);
    boutonStateOLD = boutonState;

    while (1) {
        boutonState = digitalRead(bouton);
        if (boutonState != boutonStateOLD) {
            boutonStateOLD = boutonState;
            if (boutonState == 0) { //button push
                recordState = !recordState;
                if (recordState) {
                    printf("start Recording\n");
                    recordThread = std::thread(record);

                }
                else {
                    printf("stop Recording\n");
                    system("killall arecord");
                    recordThread.join();
                }
            }
            usleep(50 * 1000);
        }
    }

    recordThread.detach();
    return 0;
}


