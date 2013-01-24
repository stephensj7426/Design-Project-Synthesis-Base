
#include "mbed.h"
#include "SDFileSystem.h"
#include "wave_player.h"


SDFileSystem sd(p5, p6, p7, p8, "sd"); //SD card

AnalogOut DACout(p18);

wave_player waver(&DACout);

int main()
{
    FILE *wave_file;
    printf("\n\n\nHello, wave world!\n");
    wave_file=fopen("/sd/sample.wav","r");
    waver.play(wave_file);
    fclose(wave_file);
}