#ifndef __UART_H
#define __UART_H

#define COM1 0
#define COM2 1

void initUART();
unsigned int getUARTStatus(int port);
char getUARTData(int port);
void resetUART();

#endif
