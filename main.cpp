#include "mbed.h"
// Class D AMP (PWM) Audio output demo for speaker
// generates a 500Hz sine wave on the analog output pin
// 128 data points on one sine wave cycle are precomputed,
// scaled, stored in an array and
// continuously output to vary the PWM duty cycle
// A very high PWM frequency is used and the average
// value of the PWM output can be used instead of a D to A

PwmOut PWM(p21);
DigitalIn button(p25);
//global variables used by interrupt routine
volatile int i=0;
float Sine_Analog_out_data[128];
float Square_Analog_out_data[128];
float Saw_Analog_out_data[128];
float Triangle_Analog_out_data[128];
enum WaveType {SINE, SQUARE, SAWTOOTH, TRIANGLE};

WaveType wave_type;

// Interrupt routine
// used to output next analog sample whenever a timer interrupt occurs
void Sample_timer_interrupt(void)
{
    if (button) {
        PWM = 0;
        return;
    }
    // send next analog sample out to D to A
    switch (wave_type)
    {
        case SINE:
            PWM = Sine_Analog_out_data[i];
            break;
        case SQUARE:
            PWM = Square_Analog_out_data[i];
            break;
        case SAWTOOTH:
            PWM = Saw_Analog_out_data[i];
            break;
        case TRIANGLE:
            PWM = Triangle_Analog_out_data[i];
            break;
    }
    // increment pointer and wrap around back to 0 at 128
    i = (i+1) & 0x07F;
}

int main()
{
    PWM.period(1.0/200000.0);
    button.mode(PullUp);
    // set up a timer to be used for sample rate interrupts
    Ticker Sample_Period;
    // precompute 128 sample points on one sine wave cycle 
    // used for continuous sine wave output later
    for(int k=0; k<128; k++) {
        Sine_Analog_out_data[k]=((1.0 + sin((float(k)/128.0*6.28318530717959)))/2.0);
        // scale the sine wave from 0.0 to 1.0 - as needed for AnalogOut arg 
        Square_Analog_out_data[k] = Sine_Analog_out_data[k] > 0.5 ? 1.0 : 0.0;

        Saw_Analog_out_data[k] = k / 128.0f;

        Triangle_Analog_out_data[k] = asin((Sine_Analog_out_data[k]*2) - 1.0);
    }
    // turn on timer interrupts to start sine wave output
    // sample rate is 500Hz with 128 samples per cycle on sine wave
    Sample_Period.attach(&Sample_timer_interrupt, 1.0/(500.0*128));
    // everything else needed is already being done by the interrupt routine

    wave_type = SINE;

    // TODO: Update synthesis for full range of values per octave (stepsize and # data points)
    while(1) {}
}