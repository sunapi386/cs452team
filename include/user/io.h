#ifndef __IO_H
#define __IO_H

#define COM1 0
#define COM2 1

struct String;

/**
Getc returns first unreturned character from the given UART.
FIXME: Discuss in documentation: How transmission errors are handled is implementation-dependent.
Getc is actually a wrapper for a send to the serial server.
Returns.
• character – success.
• -1 – if the serial server task id inside the wrapper is invalid.
• -2 – if the serial server task id inside the wrapper is not the serial
*/
int Getc(int channel);
/**
Putc queues the given character for transmission by the given UART.
On return the only guarantee is that the character has been queued.
Whether it has been transmitted or received is not guaranteed.
FIXME: Discuss in documentation: How configuration errors are handled is implementation-dependent.
Putc is actually a wrapper for a send to the serial server.
Returns.
• 0 – success.
• -1 – if the serial server task id inside the wrapper is invalid.
• -2 – if the serial server task id inside the wrapper is not the serial
server.
*/
int Putc(int channel, char c);

int PutString(int channel, struct String *s);
int GetString(int channel, struct String *s);

void receiveServer();
void sendServer();
void com1SendServer();

#endif // __IO_H
