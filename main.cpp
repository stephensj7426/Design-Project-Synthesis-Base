
#include "mbed.h"
#include "rtos.h"
#include "SDFileSystem.h"
#include "wave_player.h"


SDFileSystem sd(p5, p6, p7, p8, "sd"); //SD card

AnalogOut DACout(p18);

wave_player waver(&DACout);

int main()
{
    FILE *wave_file;
    printf("\r\n\nHello, wave world!\n\r");
    Thread::wait(1000);
    wave_file=fopen("/sd/sample.wav","r");
    if(wave_file==NULL) printf("file open error!\n\n\r");
    waver.play(wave_file);
    fclose(wave_file);
}