#ifndef __IO_H
#define __IO_H

#define COM1 0
#define COM2 1

int Getc(int channel);
int Putc(int channel, char c);
void receiveServer();

#endif // __IO_H
