#ifndef __UART_H
#define __UART_H

#define COM1 0
#define COM2 1

void initUART();
void setUARTCtrl(int event, int val);
int getUARTIntStatus(int port);
char getUARTData(int port);
void resetUART();

int getUART1ModemStatus();
void enableUART1ModemInterrupt();
void clearUART1ModemInterrupt();
void disableUART1ModemInterrupt();

#endif
