#include <lpc21xx.h>
#include <stdio.h>
typedef unsigned char U8;

void UART0_init(unsigned int baud)  
{  
	int a[] ={15,60,30,15,15};
	unsigned int result =0;  
	unsigned int PCLK =a[VPBDIV]*1000000; 
	result = PCLK /(16*baud);  
	PINSEL0 |=0X5;// UART TX/RX PIN SELECTION uart0
	U0LCR =0X83;  
	U0DLL =result & 0XFF; // lower byte result  
	U0DLM =(result >> 8) & 0XFF;//higher byte result  
	U0LCR=0X03;// DLAB =0, 8N1  
}

#define THRE ((U0LSR >>5 )&1)  

void UART0_TX( unsigned char data)  
{  
	U0THR =data; 
	while(THRE == 0);  
}
void UART0_TXS(unsigned const char *p)
{
  while(*p)
  {
    UART0_TX(*p);
    p++;
  }
}
