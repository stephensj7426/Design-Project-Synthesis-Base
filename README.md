# MBEDolin - 4180 Final Design Project
Created by Andrew Nazareth, Joshua Stephens, and Barry Walker

## Table of Contents
* [The Idea](#the-idea)
* [Hardware Setup](#hardware-setup)
* [Code](#Code)
* [Media](#media)

## The Idea
The MBEDolin is a digital synthesizer instrument that is played in a similar fashion to a violin. Instead of moving a bow across strings, the player moves their finger up and down a strip to make music. The user also has accessible buttons at their disposal to change various qualities of the sound on the fly, such as the wave type or octave of the notes.

Our team was interested in creating a synthesizer for this project, but we needed to find a way to incorporate the embedded systems elements that we have learned throughout the semester.

The inspiration for this project was this [YouTube Video](https://www.youtube.com/watch?v=MUdWeBYe3GY) by Wintergatan, in which he explains how he created the "Modulin". His system utilizes a similar potentiometer strip. However, it also uses advanced synthesizer equipment. We hope to create a device with similar functionality that uses the MBED RTOS as its core.

Besides being driven by the MBED, another way in which our device differs is that it contains an LCD screen to view the characteristics of the sound, such as the note and frequency. The screen is also used to view the current settings, such as the wave type and the current octave of the instrument.

## Hardware Setup

### Necessary Components

For the base electrical hardware of the MBEDolin, the following components are required:
* MBED
* SoftPot Membrane Potentiometer - 500mm
* uLCD Display
* Class D Audio Amplifier
* Speaker
* A Large Breadboard
* Several Pushbuttons
* A 10k Ohm Resistor

### Wiring

| MBED Pin | External 5V | SoftPot | 10k Ohm Resistor | D Class Amplifier | Speaker | uLCD Display | Pushbuttons |
| :------: | :---------: | :-----: | :--------------: | :---------------: | :-----: | :----------: | :---------: |
| Vin      | +           |         |                  | PWR+              |         | +5V          |             |
| Vout     |             | +       |                  |                   |         |              |             |
| Gnd      | -           | -       | -                | PWR-, IN-         |         | GND          | GND         |
|          |             |         |                  | OUT+              | +       |              |             |
|          |             |         |                  | OUT-              | -       |              |             |
| p13      |             |         |                  |                   |         |              | PB1         |
| p14      |             |         |                  |                   |         |              | PB2         |
| p19      |             |         |                  |                   |         |              | PB3         |
| p20      |             | Wiper   | +                |                   |         |              |             |
| p21      |             |         |                  | IN+               |         |              |             |
| p27      |             |         |                  |                   |         | TX           |             |
| p28      |             |         |                  |                   |         | RX           |             |
| p30      |             |         |                  |                   |         | RESET        |             |

When the SoftPot is not being pressed, the output is floating. Therefore, a 10k Ohm resistor is required to pull down the output to keep it defined.

### Extra Pieces

One goal of our implementation was to have it be played similarly to Wintergatan's modulin, so we added a small breadboard as well as some custom 3D printed and laser cut structural pieces.

We laser cut a piece from a sheet of acrylic to serve as the backing for the SoftPot. This backing allows us secure and press down on the SoftPot when not on a flat surface. The SoftPot came with 3M adhesive on the back, and we used that to stick it to the acrylic.

![SoftPot Front](https://github.com/stephensj7426/Design-Project-Synthesis-Base/blob/main/docs/SoftPot_Front.jpg)
![SoftPot Back](https://github.com/stephensj7426/Design-Project-Synthesis-Base/blob/main/docs/SoftPot_Back.jpg)

Next, we needed a way to attach the softpot in a manner that would allow us to carry it and the other pieces of our embedded system as one unit. Using a large black breadboard with terminal connectors as the reference, we designed a 3D printed part that could be screwed onto the breadboard to hold out the SoftPot. This part also holds the small breadboard vertically so the SoftPot can be plugged in without bending the connector. From there, the large breadboard can be held on the users shoulder and they can play the instrument similar to the Modulin.

![3D Print](https://github.com/stephensj7426/Design-Project-Synthesis-Base/blob/main/docs/3D_Printed_Holder.jpg)

## Code

### Library Dependencies
* [mbed-rtos](http://developer.mbed.org/users/mbed_official/code/mbed-rtos/#02f5cf381388)
* [4DGL-uLCD-SE](http://os.mbed.com/users/4180_1/code/4DGL-uLCD-SE/#2cb1845d768165993c6c4e2f245a16ea983a8c1f)
* [PinDetect](http://os.mbed.com/users/AjK/code/PinDetect/#cb3afc45028b380006955255db72749f92a4bfc7)

### Synthesizer Overview

#### Initial Design
The initial design for the MBEDolin relied on direct digital synthesis (DDS), sending a sample of a waveform for some set sample frequency, scaled as a 0.0 - 1.0 AnalogOut signal. After several iterations of this design, we encountered tradeoffs, which were ultimately drawbacks that we couldn't ignore. Using the DDS approach requires set sample frequency; we wanted to produce a wide range of specific musical notes, so this sample frequency needed to be fairly high. The sample frequency placed an upper bound on the frequency that we could produce. In the best case, this high interrupt frequency slowed the mbed, and prevented it from working in the worst case. If the sample frequency was lowered, the range and sound quality were reduced. 

Additionally, the DDS approach requires large steps through the sample waveform to accurately produce the necessary frequencies. This introduced problems with the quality of the output sound, such as whine and hissing. This could be remidied by increasing the quality of the sampled waveform through adding more samples. However, this required consuming more memory or introducing additional instructions in the samping interrupt service routine, which is problematic at high sample frequencies.

These tradeoffs ultimately guided us to a design that allowed us to maximize the quality of our output sounds, have a fast and responsive system, and produce other outputs concurrently.

#### Current Design
The current design for the MBEDolin uses a modified DDS, sending a sample of a waveform to an output pin; however, rather than using an AnalogOut, we use a PwmOut. We also vary the sample frequency for a set of 12 chromatic base pitches, and step twice as fast through the sample for each subsequent musical octave. 

The synthesis is handled by the create_sound() thread; first, several waveforms are generated in the mbed's RAM, so that the interrupt routine will have rapid access to the necessary sample values. These values correspond to the types of waves produced by the MBEDolin: sine, square, sawtooth, triangle, and a polyphonic square bass + sawtooth lead. Next, input values from the buttons and smooth potentiometer are read to calibrate the output sound. The smooth pot determines the pitch of the wave, while the octave buttons determine in which base octave this pitch will be produced. The current waveform is selected by the user in a menu on the uLCD, with the default being sine.

A Ticker, Sample_Period, is used to send sample frequencies to the PwmOut, PWM, at a rate of (1/(base_frequency * 128 samples)) seconds. Since our base frequencies vary from 110.0 Hz to 206.75 Hz, the sample interrupts occur from 0.07ms to 0.04ms, which is fairly rapid, but does not slow the operation of the mbed. The samples are selected from the appropriate sample array based on the currently selected wave type, and then sent to the PwmOut. The sample index is incremented based on the octave. The octave is also used to control whether the sound playing or not. If the octave is zero, no sound plays. On the transition to zero octave, the sound output decays rather than immediately jumping to zero, to smooth the sound stopping.

### RTOS Threading
The main program plays the audio and places its wave information on the LCD, through the use of separate RTOS threads. This helps the mbed stay responsive during the multiple tasks it needs to perform concurrently. The two threads are called create_sound() and display(). The system waits 0.5 seconds to check between the two threads.

#### *create_sound*
This thread is responsible for making the notes that are played by the user. During the precomputation stage, 128 sample points from the four wave types are calculated to created the lookup tables. In the while loop, the note is determined using the input from the SoftPot. This note is then bitshifted for different octaves. With the note frequency, the thread attached a function that iterates through the lookup tables is attached the Ticker, Sample_Period, at the desired frequency. This thread is ran every 50 ms.

#### *display*
This thread is responsible for updating the LCD display based on the current synthesis settings. The LCD prints out the pressed distance, frequency, octave, and current note (largest font) all in red. Towards the bottom of the screen, the LCD lists each wave name in red, with the exception of the current wave which is displayed in green. This thread is ran every 0.5 seconds.

## Media
### Mbedolin Introduction
[![Video](https://img.youtube.com/vi/rsC2MYIwLAU/0.jpg)](https://www.youtube.com/watch?v=rsC2MYIwLAU)

### Images
![Image](https://github.com/stephensj7426/Design-Project-Synthesis-Base/blob/main/docs/Display%20Unzoomed.jpg)
![Image](https://github.com/stephensj7426/Design-Project-Synthesis-Base/blob/main/docs/Top%20Down%201.jpg)
![Image](https://github.com/stephensj7426/Design-Project-Synthesis-Base/blob/main/docs/Barry%20Holding%20MBEDolin.jpg)
![Image](https://github.com/stephensj7426/Design-Project-Synthesis-Base/blob/main/docs/Josh%20Holding.jpg)


