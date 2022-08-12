#include "gpio.h"
#include <unistd.h>
#include <iostream>
#include <bcm2835.h>


#include "config.h"


void readButtonsStates(const uint8_t buttonsPins[], bool buttonsStates[]) {
	for (uint i = 0; i < 3; i++) {
		buttonsStates[i] = bcm2835_gpio_lev(buttonsPins[i]);
	}
}