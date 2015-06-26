#ifndef __LOGGER_H
#define __LOGGER_H
/**
Logger is a log server. No un-register supported.
*/

void initLogger();
void logRegister(const char *name);
void log(const char *message);

#endif
