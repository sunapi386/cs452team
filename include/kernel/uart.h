#ifndef __UART_H
#define __UART_H

#define COM1 0
#define COM2 1

void initUART();
void setUARTCtrl(int event, int val);
unsigned int getUARTStatus(int port);
char getUARTData(int port);
void resetUART();

#endif
