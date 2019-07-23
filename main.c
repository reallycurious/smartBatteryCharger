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

#include "config.h"
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
#define chargePin   4   //PORTA 4
#define ledPin      5   //PORTA 5

//pic12F1572 adc 10 bit (ref voltage 5V = 1024 => 1 unit 4,8828125 mV) 
//voltage divider R1=100K R2=33K (V0=(R1+R2/R2)*V1) v0=4,0303030303*v1
//1 unit is 4,0303030303 * 4,8828125 mV = 19,6792140151 mV ~ 20mV 
#define vUNIT 20 //20mV = 0.02v
#define vDISCONNECT 2*1000/vUNIT //2000mv (2 volt)
#define vERROR  10*1000/vUNIT //10000mv (10 volt)
#define vDELTA 80/vUNIT //80mV (Charge cut off negative delta voltage)
#define MAX_VOLTAGE 19*1000/vUNIT //19v
#define MAX_PLATEAU 15*60   //15 minutes
#define MAX_CTIME  3*60*60 //3 hours

//function defines
#define readVoltage() aRead(vChannel)
#define chargeOn() PORTA &= ~(1 << chargePin)
#define chargeOff() PORTA |= (1 << chargePin)
#define ledOn() PORTA &= ~(1 << ledPin)
#define ledOff() PORTA |= (1 << ledPin)
#define ledToggle() PORTA ^= (1 << ledPin) //^ XOR

//TEXT
const char hello[]= "Smart Charger v1.0\r\n";

//VARS
char cState = STAND_BY; //Charger state 0 (stand by)
int vPeak = 0;
short plateauCounter = 0;
int chargeCounter = 0;


void chargePWM(unsigned int duty_percent)
{       
    if (duty_percent > 100) 
        duty_percent = 50; //max %50 duty cycle
    int i = duty_percent;    
    chargeOn();
    while (i)
    {
        __delay_ms(1); // delay 1 ms
        i--;
    }    
    chargeOff();
    int i = 100 - duty_percent; // wait 100 - duty_percent ms
    while (i)
    {
        __delay_ms(1); // delay 1 ms
        i--;
    }    
}

short batteryConnected()
{
    chargeOff();    
    if (readVoltage() < vDISCONNECT) return 0;
    return 1;
}

void UART_Write_Int(int i)
{
    char text_buffer[10];
    
    sprintf(text_buffer, "%d", i);        
    UART_Write_Text((char *)text_buffer);
}

void UART_Write_Volt(int v)
{
    int dec = v/51;
    int frac =(v%51)*2*100;
    frac = frac /51;
             
    UART_Write_Int(dec);
    UART_Write_Text((char *)".");
    UART_Write_Int(frac);
}

void UART_Write_Time(int time)
{
    char hour = time/3600;    
    char minute = (time%3600)/60;
    char second = time%60;  
             
    UART_Write_Int(hour);
    UART_Write_Text((char *)":");
    UART_Write_Int(minute);
    UART_Write_Text((char *)":");
    UART_Write_Int(second);    
}

void checkDelta(int voltage)
{   
    //limit control
    if ( (plateauCounter > MAX_PLATEAU) || (chargeCounter > MAX_CTIME) || (voltage > MAX_VOLTAGE) )
        cState = TRICKLE;               
    if (plateauCounter > MAX_PLATEAU) UART_Write_Text((char *)"Max plateau\n");
    if (chargeCounter > MAX_CTIME) UART_Write_Text((char *)"Max time\n");
    if (voltage > MAX_VOLTAGE) UART_Write_Text((char *)"Max voltage\n");
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


void setup()
{
//BOOT UP HERE
    CLRWDT();   //clear watchdog timer
    __delay_ms(1000); // 1 Second Delay
    //Start serial
    UART_Init_tx_only(9600);
    UART_Write_Text((char *)hello);
    //Check if a battery is connected
    chargeOff();
    if (! batteryConnected())    
    {
        //Self Test
        chargeOn();
        __delay_ms(1000); // 1 Second Delay
        if (readVoltage() < vERROR) cState = CHRG_ERROR;
        chargeOff();
    }
//END BOOT    
}

void loop()
{
    int vCycle = 0;
    CLRWDT();   //clear watchdog timer
    switch(cState)
    {
        case STAND_BY:
            chargeOff();  
            ledOn();
            __delay_ms(500); //wait 500 ms
            if (batteryConnected() ) cState = COOL_DOWN;
            break;
        case COOL_DOWN:
            ledOff();
            vCycle = 0; //zero vCycle for averaging
            for (int iCycle = 0; iCycle < 10; iCycle++) //10*300ms is 3s
            {
                __delay_ms(300); // Wait 300 ms
                vCycle += readVoltage();                           
            }                
            vCycle = vCycle / 10;
            cState = SOFT_CHARGE;
            break;
        case SOFT_CHARGE:            
            if (chargeCounter > 4*60) //4 minutes
            {
                cState = FAST_CHARGE;
                break;
            }            
            vCycle = 0; //zero vCycle for averaging
            for (int iCycle = 0; iCycle < 10; iCycle++) //chargePWM is 100ms
            {
                chargePWM(50); //takes ~100ms
                vCycle += readVoltage();
            }
            vCycle = vCycle / 10;
            checkDelta(vCycle);
            chargeCounter ++; //add 1 sec 
            ledToggle(); //toggle at 1 second
            break;
        case FAST_CHARGE:                        
            chargeOn();
            vCycle = 0; //zero vCycle for averaging
            for (int iCycle=0; iCycle < 8; iCycle++)
            {                
                __delay_ms(125); //wait 125 ms
                vCycle += readVoltage();
                ledToggle(); //toggle at 1/8 second
            }
            chargeCounter ++; //add 1 sec            
            vCycle = vCycle >> 3; //Calculate average of 8 values (divide by 8)
            checkDelta(vCycle); 
            break;
        case TRICKLE:
            ledOn(); //Led ON
            for (int iCycle = 0; iCycle < 10; iCycle++) //chargePWM is 100ms
            {
                chargePWM(10); //takes ~100ms
                vCycle += readVoltage();
            }
            vCycle = vCycle / 10;
            checkDelta(vCycle);
            chargeCounter ++; //add 1 sec
            //Check battery
            if (! batteryConnected() ) 
                cState = STAND_BY;            
            break;
        case CHRG_ERROR:
            chargeOff();
            ledToggle(); // LED toggle
            __delay_ms(20); // Fast blink led (0.02 Second Delay)
        break;
        default:
            cState = CHRG_ERROR; //unexpected state is error
    }
    //DEBUG
    UART_Write_Text("State: ");
    UART_Write_Int(cState);    
    UART_Write_Text(" Charge: ");
    UART_Write_Time(chargeCounter);
    UART_Write_Text(" Plateau: ");
    UART_Write_Time(plateauCounter);
    UART_Write_Text(" vPeak: ");
    UART_Write_Volt(vPeak);
    UART_Write_Text(" vAvg: ");
    UART_Write_Volt(vCycle);
    UART_Write_Text("\r\n");
}

void main(void) {
    //OSCCON bit7 soft PLLx4 enable (1) bit6:3 freq (1110 = 8Mhz) 
    //bit2 unimplemented (0) bit1:0 (00 = clock src determined by config word)    
    OSCCON = 0b11110000; //internal oscillator @32Mhz
    __delay_ms(1); //let clock sck settle
    
    //WDTCON bit7:6 unimplemented bit5:1 01101 (8s) bit0: 1 (wdt enable)    
    WDTCON = 0b00011011; //set watchdog timer 8sec
    
    //WDTCON bit7:6 unimplemented bit5:1 01101 (8s) bit0: 1 (wdt enable)    
    WDTCON = 0b00011011; //set watchdog timer 8sec
    
    //ANSELA bits are default to analog after reset
    PORTA = 0; //init PORTA
    LATA = 0; //clear data latch
    ANSELA = 0; //clear ANSELA bits
    //A5 output (led), A4 output (VCTL), A3 input (MCLR), A2 input (TSENSE)
    //A1 input (VSENSE), A0 output (TX)
    TRISA = 0b00001110; 
       
    //Analog input preperation
    ANSELA = 0b00000110; //set A2 and A1 to analog input
    //FVRCON bit7 enable, bit6 ready(always 1), bit 5 temparature indicator enable, 
    //bit 4 temp indicator range selection, bit 3:2 comparator gain, bit 1:0 fvr buffer gain    
    //FVRCON = FVRCON | 0b00001111; //comparator gain 4x, fvr gain 4x
    //if ( !(FVRCON | 0b10000000)) FVRCON = FVRCON | 0b10000000; //if not enabled enable Fixed Voltage Reference
    //__delay_ms(1); //let fvr settle

    //ADCON1 bi7: justification (1 right, 0 left), bit6-5 clock select (111 FRC), 
    //bit3:2 unimplemented, bit 1:0 reference select (00 vdd-vss, 11 fvr, 10 external vref)
    ADCON1 = 0b11110000; //Right justify, FRC oscillator, Vdd and Vss Vref+ use FVR
    WPUA = WPUA & 0b11111001; //disable weak pullup on A2 and A1
     
    //Start
    setup();    //Setup
    while(1) 
    {
        loop(); //main loop
    }
//END PROGRAM          
}



