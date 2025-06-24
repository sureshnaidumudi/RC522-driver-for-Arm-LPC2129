#ifndef uart_h
#define uart_h


void UART0_init(unsigned int baud) ;
void UART0_TX( unsigned char data);
void UART0_TXS(unsigned const char *p);


#endif
