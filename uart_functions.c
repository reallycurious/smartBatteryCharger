#include "main.h"

//Configuration Bits
//SYNC    BRG16   BRGH    [BRG/EUSART Mode]   [Baud Rate Formula]
//0       0       0       8-bit/Asynchronous  F OSC /[64 (n+1)]
//0       1       0       16-bit/Asynchronous F OSC /[16 (n+1)]
//0       1       1       16-bit/Asynchronous F OSC /[4 (n+1)]


char UART_Init(const unsigned long baudrate)
{
    unsigned short n_plus; //(n+1)
    //unsigned short = error;    
    //Select 16-bit/Asynchronous with BRG16 (F OSC /[4 (n+1)])
    SYNC = 0;   //Asynchronous Mode
    BRG16 = 1;  //16Bit BRG for reduced error rate
    BRGH = 1;   //BRGH is used
    
    //sanity check
    if (baudrate >= (_XTAL_FREQ >> 2) ) //impossible baud
        return 0;   //failed
    //Calculate SPBRGH:SPBRGL    
    n_plus = (_XTAL_FREQ >> 2)/baudrate;
    //error = (_XTAL_FREQ >> 2)%baudrate;
            
    SPBRG = n_plus -1;  //Select Baud Rate (SPBRGH:SPBRGL)
    SPEN = 1;   //Serial Port Enable
    CREN = 1;   //Continuous Receive Enable (0 = Disable receive)
    TXEN = 1;   //Transmit Enable
    return 1;   //success                
}

char UART_Init_tx_only(const unsigned long baudrate)
{
    UART_Init(baudrate);
    CREN = 0;   //Continuous Receive Enable (0 = Disable receive)
}

char UART_Read()
{
  while(!UART_Data_Ready); //wait for receive buffer to fill
  return RCREG;
}

void UART_Write(char data)
{
  while(!UART_TX_Empty); //wait for send buffer to empty
  TXREG = data;
}
