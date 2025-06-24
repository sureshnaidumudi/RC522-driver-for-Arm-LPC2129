#ifndef spi_h
#define spi_h

void spi_init(void);
char spi0_tx(unsigned char data);
#define cs_pin ( 1 << 7)

#endif
