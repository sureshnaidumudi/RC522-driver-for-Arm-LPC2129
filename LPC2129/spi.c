#include <lpc21xx.h>

void spi_init(void)
{
	PINSEL0|=0X1500;
	IODIR0|=1<<7;
	IOSET0=1<<7;
	S0SPCR=0X20;
	S0SPCCR=15;
}

#define SPIF ((S0SPSR>>7)&1)

char spi0_tx(unsigned char data)
{
	S0SPDR=data;
	while(SPIF==0);
	return S0SPDR;
}
