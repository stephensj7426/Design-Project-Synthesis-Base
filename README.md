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

The inspiration for this project was this [YouTube Video](https://www.youtube.com/watch?v=MUdWeBYe3GY) by Wintergatan, in which he creates the "Modulin". His system utilizes a similar potentiometer strip. However, it also uses advanced sythesizer equipment. We hope to create a device with similar functionality that uses the MBED RTOS as its core.

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

We want our implementation to be played similarly to Wintergatan's modulin, so we added a small breadboard and some custom 3D printed structural pieces.

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

## Code

### Library Dependencies
* [mbed-rtos](http://developer.mbed.org/users/mbed_official/code/mbed-rtos/#02f5cf381388)
* [4DGL-uLCD-SE](http://os.mbed.com/users/4180_1/code/4DGL-uLCD-SE/#2cb1845d768165993c6c4e2f245a16ea983a8c1f)
* [PinDetect](http://os.mbed.com/users/AjK/code/PinDetect/#cb3afc45028b380006955255db72749f92a4bfc7)

### Synthesizer Overview

    Example code here

### RTOS Threading


## Media
