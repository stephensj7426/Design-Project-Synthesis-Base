#include "mbed.h"
// Cylon style LED scanner
BusOut led_state(LED1, LED2, LED3, LED4);

int main() {
    int i = 0;
    // initinal LED state
    led_state = 0x1;
    while (1) {
        // loop through all states
        for (i=0; i<6; i++) {
            if (i<3)
                // shift left 1 bit until high LED set
                led_state = led_state << 1;
            else
                // then reverse and shift back to right 1 bit
                led_state = led_state >> 1;
            // time delay .1s to slow down display
            wait(0.1);
        }
    }
}
