#include "mbed.h"
#include "rtos.h"
#include "uLCD_4DGL.h"

#define DEBUG 0

// TODO: Add LCD display effects, buttons to control octave, smooth pot handling, more output effects

// Uses Class D Audio Amp to produce output waves of varying types

// Currently reads Pot to select note, will use different analog sensor

PwmOut PWM(p21);
DigitalIn button(p25);
AnalogIn pot(p20);
uLCD_4DGL lcd(p28, p27, p30);
DigitalOut LED(LED1);

Mutex lcd_mutex;

Ticker Sample_Period;


//global variables used by interrupt routine
volatile int i = 0;
volatile int octave = 1; // Octave select, must be power of 2 & > 0
volatile int octave_base = 1;

volatile float current_frequency;

// Pre-computed wave output arrays
float Sine_Analog_out_data[128];
float Square_Analog_out_data[128];
float Saw_Analog_out_data[128];
float Triangle_Analog_out_data[128];
float saw_lead_bass[128];

// Type of wave
enum WaveType {W_SINE, W_SQUARE, W_SAWTOOTH, W_TRIANGLE, W_DUAL};
volatile WaveType wave_type;

// Note base frequencies, used for base pitch generation
const float base_freqs[] = {110.0f, 116.54f, 123.47f, 130.81f, 138.59f, 146.83f, 155.56f, 164.81f, 174.61f, 185.00f, 195.00f, 206.75f};

// Interrupt routine
// used to output next analog sample whenever a timer interrupt occurs
void Sample_timer_interrupt(void)
{
    //if (button) {
    //    PWM = 0;
    //    return;
    //}
    // send next analog sample out to D to A, based on type of wave
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

void create_sound(void const *argument) {
	// precompute 128 sample points on one wave cycle for each wave type
	for(int k=0; k<128; k++) {
		// Sine & scale
		Sine_Analog_out_data[k]=((1.0 + sin((float(k)/128.0*6.28318530717959)))/2.0);
		// Square, based on sine value 
		Square_Analog_out_data[k] = Sine_Analog_out_data[k] > 0.5 ? 1.0 : 0.0;
		// Sawtooth, based on index in array
		Saw_Analog_out_data[k] = k / 128.0f;
		// Triange - arcsine of the unscaled sine value
		Triangle_Analog_out_data[k] = asin((Sine_Analog_out_data[k]*2) - 1.0);

		saw_lead_bass[k] = ((Square_Analog_out_data[k]/4.0 + (k%42)/41.0))/1.25;
	}
	// Initialize wave type to default
	wave_type = W_SQUARE;
    octave_base = 2;
    octave = octave_base;

	// Enable sample interrupts - 110 Hz wave with 128 samples per cycle
	Sample_Period.attach(&Sample_timer_interrupt, 1.0/(110.0*128));

	// Init smooth pot value & old val for note
	double smooth_pot = 0.0;
	int old_note_val = 0;
	while (1) {
		// Smooth the pot value to reduce noise
        smooth_pot = 0.9 * smooth_pot + 0.1 * pot;
        // Calculate tne new note index - one of 12 chromatic pitches
        int new_val = (int) (smooth_pot * 24);
        // If the note has changed (reduces stopping/starting of interrupts)
        if (old_note_val != new_val) {
            old_note_val = new_val;
            if (new_val > 11) {
                octave = octave_base << 1;
            } else {
                octave = octave_base;
            }
            // Detatch the old sample interrupts
            Sample_Period.detach();
            //Calculate new frequency
            current_frequency = base_freqs[new_val % 12];
            // Start new interrupt with new base frequency
            Sample_Period.attach(&Sample_timer_interrupt, 1.0/(current_frequency * 128));
        }
		Thread::wait(50.0);
	}
}

void display(void const *argument) { 
	while(1) {
		lcd_mutex.lock();
		lcd.color(RED);
        lcd.locate(0,3);
        float val = pot;
		lcd.printf("Distance: %.2f", val * 500);
        lcd.locate(0, 4);
        lcd.printf("Freq: %.2f", current_frequency);
		if (octave == 1) {
			lcd.color(BLUE);
			lcd.locate(2,2);
			lcd.printf("Octave");
		}
		lcd_mutex.unlock();
		Thread::wait(500.0);
	}
}

int main()
{
    lcd.baudrate(BAUD_1500000);
	lcd.text_height(2);
    lcd.text_width(2);
    lcd.printf("MBEDolin");
    lcd.text_height(1);
    lcd.text_width(1);
    LED = 0;
    printf("INIT\n");
    // Init PWM period, button mode, and Ticker for interrupts
    PWM.period(1.0/200000.0);
    PWM = 0;
    button.mode(PullUp);

	Thread thread2(create_sound);
	Thread thread3(display);
    // Main loop
    while(1) {
        LED = !LED;
		Thread::wait(1000.0);
    }
}
