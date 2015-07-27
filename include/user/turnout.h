#ifndef __TURNOUT_H
#define __TURNOUT_H
#include <utils.h> // bool
// handles turnout manipulation, and draws onto screen

void initTurnouts();
// sets the turnout curved or straight

bool turnoutIsCurved(int turnout_number);
void printResetTurnouts();
void setTurnout(int turnout_number, char direction);

#endif
