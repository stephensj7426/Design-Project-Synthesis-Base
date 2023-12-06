#include "mbed.h"
#include "rtos.h"
#include "uLCD_4DGL.h"
#include "PinDetect.h"
#include <cstdlib>

// Uses Class D Audio Amp to produce output waves of varying types

// Reads 500 mm smooth pot to determine chromatic note; 2 octaves on pot

// Init I/O Pins
PwmOut PWM(p21);
AnalogIn pot(p20);
uLCD_4DGL lcd(p28, p27, p30);
DigitalOut LED(LED1);
DigitalOut led2(LED2);
PinDetect waveswitch(p19);
PinDetect oct_b1(p13);
PinDetect oct_b0(p14);

Ticker Sample_Period;


//global variables used by interrupt routines/Threads
volatile int i = 0;
volatile int octave = 1; // Octave select, must be power of 2; 0 octave = silence
volatile int octave_base = 1;
volatile int offset = 0;

volatile float current_frequency;

// Pre-computed wave output arrays
float Sine_Analog_out_data[128];
float Square_Analog_out_data[128];
float Saw_Analog_out_data[128];
float Triangle_Analog_out_data[128];
float saw_lead_bass[128];

// Type of wave
enum WaveType {W_SINE, W_SQUARE, W_SAWTOOTH, W_TRIANGLE, W_DUAL};
volatile int wave_type;

// Simulated decay of PWM signal
volatile float pwm_decay;

// Note base frequencies and names, used for base pitch generation
const float base_freqs[] = {110.0f, 116.54f, 123.47f, 130.81f, 138.59f, 146.83f, 155.56f, 164.81f, 174.61f, 185.00f, 195.00f, 206.75f};
const char* notes[] = {"A ", "Bb", "B ", "C ", "Db", "D ", "Eb", "E ", "F ", "Gb", "G ", "Ab"};

//Values needed for the wave type selection menu
const int TOP_WAVE_COL = 8;
const char* wave_strings[] = {"SINE", "SQUARE", "SAWTOOTH", "TRIANGLE", "DUAL"};
volatile int prev_wave;
volatile bool wave_changed = false;

// Utility function; Simple log_2 algorithm
int simple_log_2(int value)
{
    int result = 0;
    while (value > 0)
    {
        result += 1;
        value>>=1;
    }
    return result;
}

// Callback function for setting bit 0 of octave offset
void b0_int(void)
{
    offset |= 1;
}

// Callback function for setting bit 1 of octave offset
void b1_int(void)
{
    offset |= 2;
}

// Callback function for unsetting bit 0 of octave offset
void b0_unint(void)
{
    offset &= (~1);
}

// Callback function for unsetting bit 1 of octave offset
void b1_unint(void)
{
    offset &= (~2);
}

// Interrupt routine
// used to output next analog sample whenever a timer interrupt occurs
void Sample_timer_interrupt(void)
{
    // If octave is 0, silence or begin decay
    if (!octave) {
        if (i != 0) {
            pwm_decay = Sine_Analog_out_data[i];
            i = 0;
        } else {
            pwm_decay /= 2;
            if (pwm_decay < 0.0000005)
                pwm_decay = 0;
        }
        PWM = pwm_decay;
        return;
    }
    // send next PWM sample out to speaker, based on type of wave
    switch (wave_type)
    {
        case W_SINE:
            PWM = Sine_Analog_out_data[i];
            break;
        case W_SQUARE:
            PWM = Square_Analog_out_data[i];
            break;
        case W_SAWTOOTH:
            PWM = Saw_Analog_out_data[i];
            break;
        case W_TRIANGLE:
            PWM = Triangle_Analog_out_data[i];
            break;
        case W_DUAL:
            PWM = saw_lead_bass[i];
            break;
    }
    // increment pointer (depending on octave) and wrap around back to 0 at 128
    i = (i+1*octave) & 0x7F;
}

// Thread responsible for sound init and continued playing
void create_sound(void const *argument) {
	// precompute 128 sample points on one wave cycle for each wave type
	for(int k=0; k<128; k++) {
		// Sine & scale
		Sine_Analog_out_data[k]=((1.0 + sin((float(k)/128.0*6.28318530717959)))/2.0);
		// Square, based on sine value 
		Square_Analog_out_data[k] = Sine_Analog_out_data[k] > 0.5 ? 0.75 : 0.0;
		// Sawtooth, based on index in array
		Saw_Analog_out_data[k] = k / 128.0f;
		// Triange - arcsine of the unscaled sine value
		Triangle_Analog_out_data[k] = asin((Sine_Analog_out_data[k]*2) - 1.0);
        // Square Bass(fundamental), reduced amplitude + Saw Lead(3/2 fundamental)
		saw_lead_bass[k] = ((Square_Analog_out_data[k]/4.0 + (k%42)/41.0))/1.25;
	}
	// Initialize wave type to default
	wave_type = W_SINE;
    octave_base = 1;
    octave = octave_base;

	// Enable sample interrupts - 110 Hz wave with 128 samples per cycle
	Sample_Period.attach(&Sample_timer_interrupt, 1.0/(110.0*128));

	// Init old val for note and octave base, minimizes switching
	int old_note_val = 0;
    int old_octave_base = 1;
	while (1) {
        // Calculate tne new note index - one of 24 chromatic pitches
        int new_val = (int) (pot * 25);
        // Read offset and set octave base accordingly
        switch (offset) {
            case 0:
                octave_base = 1;
                break;
            case 1:
                octave_base = 2;
                break;
            case 2:
                octave_base = 8;
                break;
            case 3:
                octave_base = 16;
                break;
        }
        // Free up CPU if we're not playing
        if (octave_base == 0 || new_val == 0) {
            old_note_val = -1;
            Sample_Period.detach();
            Thread::yield();
            Thread::wait(50);
            continue;
        }
        new_val--;
        // If the note has changed (reduces stopping/starting of interrupts)
        if (old_note_val != new_val || old_octave_base != octave_base) {
            old_octave_base = octave_base;
            old_note_val = new_val;
            // If note in second octave of smooth pot, double octave
            if (new_val > 11) {
                octave = octave_base << 1;
            } else {
                octave = octave_base;
            }
            //Calculate new frequency
            current_frequency = base_freqs[new_val % 12];
            // Start new interrupt with new base frequency
            Sample_Period.attach(&Sample_timer_interrupt, 1.0/(current_frequency * 128));
        }
        // Delay 50ms - want this to be fairly small and responsive
		Thread::wait(50.0);
	}
}

// Thread for updating the LCD display based on current synth values
void display(void const *argument) { 
	while(1) {
        // Print distance
		lcd.color(RED);
        lcd.locate(0,3);
        float val = pot;
		lcd.printf("Distance: %.2f", val * 500);

        // Frequency
        lcd.locate(0, 4);
        lcd.printf("Freq: %.2f", current_frequency * octave);

        // Octave (1-8 in ones, not powers of 2)
		lcd.locate(0,5);
		lcd.printf("Octave: %d", simple_log_2(octave));

        // Print out the current note in large font
        lcd.locate(9, 10);
        lcd.text_height(5);
        lcd.text_width(5);
        int note_idx = (int) (pot * 25) == 0 ? -1 : (((int) (pot * 25)) - 1) % 12;
        lcd.printf("%s", note_idx == -1 ? "  " : notes[note_idx]);
        lcd.text_height(1);
        lcd.text_width(1);

        // If the wave was changed, update the selected wave on the menu
        if (wave_changed) {
            lcd.color(BLUE);
            lcd.locate(0, TOP_WAVE_COL - 1);
            lcd.printf("WAVE TYPE:");

            lcd.color(GREEN);
            lcd.locate(0, wave_type + TOP_WAVE_COL);
            lcd.printf(wave_strings[wave_type]);

            lcd.color(RED);
            lcd.locate(0, prev_wave + TOP_WAVE_COL);
            lcd.printf(wave_strings[prev_wave]);

            wave_changed = false;
        }

        // Delay
		Thread::wait(500.0);
	}
}

// Thread to cycle through currently selected wave in enum
void switch_wave() {
    prev_wave = wave_type;
    wave_type = (wave_type + 1) % 5;
    wave_changed = true;
}

// Main thread, init others, then show LED alive
int main()
{
    // Set up LCD and display title
    lcd.baudrate(BAUD_1500000);
	lcd.text_height(2);
    lcd.text_width(2);
    lcd.printf("MBEDolin");
    lcd.text_height(1);
    lcd.text_width(1);

    LED = 0;

    // Set up octave button interrupts
    oct_b1.mode(PullUp);
    oct_b0.mode(PullUp);
    oct_b0.attach_deasserted(&b0_int);
    oct_b1.attach_deasserted(&b1_int);
    oct_b0.attach_asserted(&b0_unint);
    oct_b1.attach_asserted(&b1_unint);
    oct_b1.setSampleFrequency();
    oct_b0.setSampleFrequency();

    // Init PWM period, button mode, and Ticker for interrupts
    PWM.period(1.0/200000.0);
    PWM = 0;

    // Init wave switch button
    waveswitch.mode(PullUp);
    waveswitch.attach_deasserted(&switch_wave);
    waveswitch.setSampleFrequency();

    // Start threads
	Thread thread2(create_sound);
	Thread thread3(display);

    // Initialize the wave type text
    for (int i = 0; i < 6; i++) {
        switch_wave();
        while (wave_changed) {
            Thread::wait(100.0);
        }
    }

    // Still-alive loop
    while(1) {
        LED = !LED;
		Thread::wait(500.0);
    }
}
