/*
 * File:   main.c
 * Author: storm
 *
 * Created on March 9, 2019, 5:36 PM
 */

// PIC12F1572 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1
#pragma config FOSC = INTOSC    //  (INTOSC oscillator; I/O function on CLKIN pin)
#pragma config WDTE = ON        // Watchdog Timer Enable (WDT enabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = ON       // MCLR Pin Function Select (MCLR/VPP pin function is MCLR)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = OFF      // PLL Enable (4x PLL disabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LPBOREN = OFF    // Low Power Brown-out Reset enable bit (LPBOR is disabled)
#pragma config LVP = ON         // Low-Voltage Programming Enable (Low-voltage programming enabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

//_XTAL_FREQ defined in main.h

#include "main.h"
#include <xc.h>
#include <pic12f1572.h>
#include <stdint.h>
#include <stdio.h>

//Charger states
#define STAND_BY    0
#define COOL_DOWN   1
#define SOFT_CHARGE 2
#define FAST_CHARGE 3
#define TRICKLE     4
#define CHRG_ERROR  255

//Other
#define vChannel    1   //channel for voltage read
#define chargePin   4   //PORTA 4 and PWM2 is reassigned to A4 in code
#define pwmChannel  2
#define ledPin      5   //PORTA 5

//pic12F1572 adc 10 bit (ref voltage 5V = 1024 => 1 unit 4,8828125 mV) 
//voltage divider R1=100K R2=33K (V0=(R1+R2/R2)*V1) v0=4,0303030303*v1
//1 unit is 4,0303030303 * 4,8828125 mV = 19,6792140151 mV ~ 20mV 
#define vUNIT 20 //20mV = 0.02v
#define vDISCONNECT 2*1000/vUNIT //2000mv (2 volt)
#define vERROR  10*1000/vUNIT //10000mv (10 volt)
#define vDELTA 60/vUNIT //60mV (Charge cut off negative delta voltage)
#define MAX_VOLTAGE 19*1000/vUNIT //19v
#define MAX_PLATEAU 15*60   //15 minutes
#define MAX_CTIME  3*60*60 //3 hours

//function defines
#define readVoltage() aRead(vChannel)
#define chargeOn() chargePWM(255)
#define chargeOff() chargePWM(0)
#define ledOn() PORTA &= ~(1 << ledPin)
#define ledOff() PORTA |= (1 << ledPin)
#define ledToggle() PORTA ^= (1 << ledPin) //^ XOR

//TEXT
const char hello[]= "Smart Charger v1.0\r\n";

//VARS
char cState = STAND_BY; //Charger state 0 (stand by)
unsigned short vPeak = 0;
unsigned short plateauCounter = 0;
unsigned long chargeCounter = 0;


void chargePWM(unsigned char duty)
{           
    duty = ~duty; //reverse duty    
    if (duty == 0)
        PORTA &= ~(1 << chargePin);  
    else if (duty == 255)
        PORTA |= (1 << chargePin); 
    else
        initPWM(pwmChannel, duty); 
}

char batteryConnected()
{
    chargeOff();    
    if (readVoltage() < vDISCONNECT) return 0;
    return 1;
}

void Serial_Write_Text(char *text)
{
  short i = 0;
  while (text[i] != '\0')
  {
      UART_Write(text[i]);
      i ++;
  }   
}

void Serial_Write_Int(int i)
{
    char text_buffer[10];
    
    sprintf(text_buffer, "%d", i);        
    Serial_Write_Text((char *)text_buffer);
}

void Serial_Write_Volt(int v)
{
    int dec = v/51;
    int frac =(v%51)*2*100;
    frac = frac /51;
             
    Serial_Write_Int(dec);
    Serial_Write_Text((char *)".");
    Serial_Write_Int(frac);
}

void Serial_Write_Time(int time)
{
    char hour = time/3600;    
    char minute = (time%3600)/60;
    char second = time%60;  
             
    Serial_Write_Int(hour);
    Serial_Write_Text((char *)":");
    Serial_Write_Int(minute);
    Serial_Write_Text((char *)":");
    Serial_Write_Int(second);    
}

void checkDelta(int voltage)
{   
    //limit control
    if ( (plateauCounter > MAX_PLATEAU) || (voltage > MAX_VOLTAGE) )
        cState = TRICKLE;               
    //routine
    if (voltage > vPeak) 
            {
                plateauCounter = 0; //zero plateau counter
                vPeak = voltage;
            }                
    else if ( (vPeak - voltage) > vDELTA) 
        cState = TRICKLE;
    else //vCycle <= vPeak -> start timer                 
        plateauCounter++;    
}

void delay_1ms()
{
    __delay_ms(1); // Wait 1 ms
}

void delay_125ms()
{
    __delay_ms(125); // Wait 125 ms
}

void delay_1s()
{
    __delay_ms(1000); // Wait 1 s
}

void setup()
{
//BOOT UP HERE
    CLRWDT();   //clear watchdog timer
    delay_1ms(); // 1 ms Delay
    initADC(vChannel);
    //Start serial
    UART_Init_tx_only(9600);
    Serial_Write_Text((char *)hello);
    //Check if a battery is connected
    chargeOff();
    if (! batteryConnected())    
    {
        //Self Test
        chargeOn();
        delay_1s(); // 1 Second Delay
        if (readVoltage() < vERROR) cState = CHRG_ERROR;
        chargeOff();
    }
//END BOOT    
}

void loop()
{
    short vCycle = 0; //zero vCycle for averaging
    CLRWDT();   //clear watchdog timer
    switch(cState)
    {
        case STAND_BY:
            chargeOff();  
            ledOn();
            delay_1s();
            if (batteryConnected() ) 
                cState = COOL_DOWN;
            break;
        case COOL_DOWN: 
            ledOff();            
            for (char iCycle = 0; iCycle < 8; iCycle++) 
            {
                delay_125ms(); // Wait 125 ms
                vCycle += readVoltage();                           
            }                            
            vCycle = vCycle >> 3; //divide by 8                
            chargeCounter++;
            if (chargeCounter > 2) //3 seconds
                cState = SOFT_CHARGE;
            break;
        case SOFT_CHARGE:   //C/10 charge 
            ledToggle(); //toggle at 1 second
            chargePWM(76); // 76/256*400 ma 118.75 ma (C=1200 ma C/10=120 ma)            
            for (char iCycle = 0; iCycle < 8; iCycle++) 
            {                
                delay_125ms(); //wait 125 ms
                vCycle += readVoltage();
            }
            vCycle = vCycle >> 3; //divide by 8
            chargeCounter ++; //add 1 sec             
            checkDelta(vCycle);            
            if (chargeCounter > 4*60) //4 minutes
                cState = FAST_CHARGE;
            break;
        case FAST_CHARGE:                        
            chargeOn();            
            for (char iCycle=0; iCycle < 8; iCycle++)
            {                
                delay_125ms(); //wait 125 ms
                vCycle += readVoltage();
                ledToggle(); //toggle at 1/8 second
            }
            vCycle = vCycle >> 3; //Calculate average of 8 values (divide by 8)
            chargeCounter ++; //add 1 sec                        
            checkDelta(vCycle); 
            if (chargeCounter > MAX_CTIME)
                cState = TRICKLE;
            break;
        case TRICKLE:   //C/50 - C/20 charge
            ledOn(); //Led ON
            chargePWM(16); // 16/256*400ma = 25ma (C=1200ma C/50=24 ma)
            for (char iCycle = 0; iCycle < 8; iCycle++) //chargePWM is 100ms
            {                
                delay_125ms(); //wait 125 ms
                vCycle += readVoltage();
            }
            vCycle = vCycle >> 3; //divide by 8
            checkDelta(vCycle);
            chargeCounter ++; //add 1 sec
            //Check battery
            if (! batteryConnected() ) 
                cState = STAND_BY;            
            break;
        case CHRG_ERROR:
            chargeOff();            
            ledToggle(); // LED toggle
            for (char iCycle=0; iCycle < 20; iCycle++)
                delay_1ms(); // Fast blink led (0.02 Second Delay)
        break;
        default:
            cState = CHRG_ERROR; //unexpected state is error
    }
    //DEBUG
    Serial_Write_Text("State: ");
    Serial_Write_Int(cState);    
    Serial_Write_Text(" Charge: ");
    Serial_Write_Time(chargeCounter);
    Serial_Write_Text(" Plateau: ");
    Serial_Write_Time(plateauCounter);
    Serial_Write_Text(" vPeak: ");
    Serial_Write_Volt(vPeak);
    Serial_Write_Text(" vAvg: ");
    Serial_Write_Volt(vCycle);
    Serial_Write_Text("\r\n");
}

void main(void) {
    //OSCCON bit7 soft PLLx4 enable (1) bit6:3 freq (1110 = 8Mhz) 
    //bit2 unimplemented (0) bit1:0 (00 = clock src determined by config word)    
    OSCCON = 0b11110000; //internal oscillator @32Mhz
    delay_1ms(); //let clock sck settle
    
    //WDTCON bit7:6 unimplemented bit5:1 01101 (8s) bit0: 1 (wdt enable)    
    WDTCON = 0b00011011; //set watchdog timer 8sec
    
    //ANSELA bits are default to analog after reset
    PORTA = 0; //init PORTA
    LATA = 0; //clear data latch
    ANSELA = 0; //clear ANSELA bits
    //A5 output (led), A4 output (VCTL), A3 input (MCLR), A2 input (TSENSE)
    //A1 input (VSENSE), A0 output (TX)
    TRISA = 0b00001110; 
        
    //APFCON: ALTERNATE PIN FUNCTION CONTROL REGISTER
    //7         6       5       4   3       2       1       0
    //RXDTSEL   CWGASEL CWGBSEL ?   T1GSEL  TXCKSEL P2SEL   P1SEL
    APFCON |= 0b00000010; //Select A4 Alternate function PWM2
    SLRCONA &= 0b00100111; //disable slew rate limiting for A4
            
//Start
    setup();    //Setup
        while(1) 
    {        
        loop(); //main loop
    }
//END PROGRAM          
}



