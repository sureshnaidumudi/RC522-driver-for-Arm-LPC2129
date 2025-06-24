#include <LPC21xx.H>
#include <stdio.h>
#include "delay.h"
#include "spi.h"
#include "uart.h"
#include "RC522.h"

extern Uid uid;
unsigned char cardData[15];
unsigned char sendBuffer[30];
byte readUID[5];

void formatUID() {
    int i;
    int index = 0;
    for (i = 0; i < 4; i++) {
        index += sprintf(&cardData[index], "%02X", readUID[i]);     // Convert each byte to Hex
    }
    cardData[index] = '\0';                                         // Null terminate formatted UID
}

void prepareSendBuffer() {
	sprintf(sendBuffer, "AT+SEND=%s", cardData);                    // Concatenate "AT+SEND=" with cardData
}

void sendData() {
    UART0_TXS(sendBuffer);                                          // Send full command
    UART0_TX(13);                                                   // Send Carriage Return '\r'
    UART0_TX(10);                                                   // Send Line Feed '\n'
}

int main()
{
    byte i;
    spi_init();
    UART0_init(9600);
    RC522_init();
    IODIR0 |= 1 << 21 ;
    IOCLR0 |= 1 << 21 ;
    while(1)
    {
    if (!RC522_IsNewCardPresent() || !RC522_ReadCardSerial())
    {
        continue;
    }
    IOSET0 |= 1 << 21 ;                                         // Beep the Buzzer ( Turn on )
    delay_ms(500);
    IOCLR0 |= 1 << 21 ;                                         // Turn off the buzzer
    for(i=0 ; i < 4 ; i++)
    {
        readUID[i] = uid.uidByte[i];
    }
    formatUID();                                                // Format UID (removes spaces)
    prepareSendBuffer();                                        // Prepare full AT command
    sendData();                                                 // Transmit to ESP8266
    RC522_HaltA();
    RC522_StopCrypto1();
    }
}
