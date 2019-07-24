#include "main.h"

char initADC(char channel)
{
    if (channel > 3)
        return 0; //only channels 0-3 available
    //Analog input preperation    
    TRISA |= (1 << channel); //set channel to input
    ANSELA |= (1 << channel); //set channel to analog input
    //FVRCON bit7 enable, bit6 ready(always 1), bit 5 temparature indicator enable, 
    //bit 4 temp indicator range selection, bit 3:2 comparator gain, bit 1:0 fvr buffer gain    
    //FVRCON = FVRCON | 0b00001111; //comparator gain 4x, fvr gain 4x
    //if ( !(FVRCON | 0b10000000)) FVRCON = FVRCON | 0b10000000; //if not enabled enable Fixed Voltage Reference
    //__delay_ms(1); //let fvr settle

    //ADCON1 bi7: justification (1 right, 0 left), bit6-5 clock select (111 FRC), 
    //bit3:2 unimplemented, bit 1:0 reference select (00 vdd-vss, 11 fvr, 10 external vref)
    ADCON1 = 0b11110000; //Right justify, FRC oscillator, Vdd and Vss Vref+ use FVR
    WPUA &= ~(1 << channel); //disable weak pullup on channel
    return 1;
}

short aRead(unsigned char channel)
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

#define PWM_CONFIG      0b11100000  //EN=1, OE=1, OUT=1, POL=1, MODE=00, unimplemented=00
#define PWM_CLKCONFIG   0b00000000; //PRESCALER = /1(bits 6:5:4) CLK_SRC=F_OSC
#define PWM_PERIOD      ( (_XTAL_FREQ ) / freq) - 1 //((F_OSC/1)/freq)-1
#define PWM_PHASE       0
#define PWM_OFFSET_CFG  0b00000001;
#define PWM_DUTY        ((PWM_PERIOD +1)*duty >> 8) //duty 0-255


char initPWM(char channel, unsigned char duty)
{    
    unsigned long freq = ( (_XTAL_FREQ ) >> 8); //maximum frequency divisible by 256
    //Set pwm period    
    switch(channel)
    {
        case 1:
            PWM1CON = PWM_CONFIG;
            PWM1CLKCON = PWM_CLKCONFIG;
            PWM1PR = PWM_PERIOD;
            PWM1PH = PWM_PHASE;
            PWM1DC = PWM_DUTY;
            //PWM1OFCON = PWM_OFFSET_CFG;
            break;
        case 2:
            PWM2CON = PWM_CONFIG;
            PWM2CLKCON = PWM_CLKCONFIG;
            PWM2PR = PWM_PERIOD;
            PWM2PH = PWM_PHASE;
            PWM2DC = PWM_DUTY;
            //PWM2OFCON = PWM_OFFSET_CFG;
            break;
        case 3 :
            PWM3CON = PWM_CONFIG;
            PWM3CLKCON = PWM_CLKCONFIG;
            PWM3PR = PWM_PERIOD;
            PWM3PH = PWM_PHASE;
            PWM3DC = PWM_DUTY;
            //PWM3OFCON = PWM_OFFSET_CFG;
            break;
        default: return 0; //illegal channel
    }
    return 1;
}

void stopPWM(char channel)
{
    switch(channel)
    {
        case 1:
            PWM1CON &= ~(1 << 7); //disable
            break;
        case 2:
            PWM2CON &= ~(1 << 7); //disable
            break;            
        case 3:
            PWM3CON &= ~(1 << 7); //disable
            break;                    
    }
}