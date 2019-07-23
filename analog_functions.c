#include "config.h"

short aRead(char channel)
{
    short result; 
    
    //ADCON bits 6:2 are channel select AN0=0b00000000, AN1=0b00000100 
    //AN2=0b00001000, AN3=0b00001100, Temp=0b01110100
    //ADCON bit 1 is GO/DONE and bit 0 is ADON (enable ADC)
    ADCON0 = channel << 2; //set channel select bits
    ADCON0 = ADCON0 | 0b00000001; //(ADON); //turn ADC on    
     __delay_ms(1); //acquisition delay (actually microseconds)
    ADCON0 = ADCON0 | 0b00000010; //(ADGO); //start conversion
    while (ADCON0 & 0b00000010) //(ADGO) )  
        {
        //wait for conversion 
        }
    ADCON0 = ADCON0 & 0b11111110; //turn ADC off
    result = (ADRESH << 8) | ADRESL; //combine two 8bit values into a 16bit val 
    return (result);
}

//Standard mode (MODE<1:0> = 00)
//PWM PERIOD IN STANDARD MODE
//Period = (PWMxPR + 1)*Prescale / PWMxCLK
//Duty Cycle = (PWMxDC ? PWMxPH) / (PWMxPR + 1)
//Independent Run mode (OFM<1:0> = 00)

//PWMxCON: PWMx CONTROL REGISTER
//7   6   5   4   3   2       1   0
//EN  OE  OUT POL MODE<1:0>   unimplemented
//PWMxCLKCON: PWMx CLOCK CONTROL REGISTER
//7             6   5   4       3       2       1       0
//unimplemented prescaler[2:0]  unimplemented   clock_source[1:0]
//111 = Divides clock source by 128             00 = FOSC

#define PRESCALE    _XTAL_FREQ >> 7

void initPWM(unsigned long freq, char duty, char channel)
{    
    if (freq > ( (_XTAL_FREQ >> 7)/100) ) 
        freq = (_XTAL_FREQ >> 7)/100;
    //Set pwm period    
    switch(channel)
    {
        case 1:
            PWM1CON = 0b11110000; 
            PWM1CLKCON = 0b01110000; // FOSC/128 (FOSC >> 7)
            PWM1PR = ( (_XTAL_FREQ >> 7) / freq) - 1;
            PWM1PH = 0; //phase 0
            PWM1DC = (PWM1PR +1)*duty/100;
            break;
        case 2:
            PWM2CON = 0b11110000;
            PWM2CLKCON = 0b01110000;
            PWM2PR = ( (_XTAL_FREQ >> 7) / freq) - 1;
            PWM2PH = 0; //phase 0
            PWM2DC = (PWM2PR +1)*duty/100;
            break;
        case 3 :
            PWM3CON = 0b11110000;
            PWM3CLKCON = 0b01110000;
            PWM3PR = ( (_XTAL_FREQ >> 7) / freq) - 1;
            PWM3PH = 0; //phase 0
            PWM3DC = (PWM3PR +1)*duty/100;            
            break;
        default: return; //illegal channel
    }
}