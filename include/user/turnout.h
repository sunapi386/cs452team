#ifndef __TURNOUT_H
#define __TURNOUT_H
#include <utils.h> // bool
// handles turnout manipulation, and draws onto screen

void initTurnout();
// sets the turnout curved or straight

void turnoutSet(int turnout_number, bool desire_curved);
bool turnoutIsCurved(int turnout_number);

#endif
