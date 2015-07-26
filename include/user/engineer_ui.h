#ifndef __ENGINEER_UI_H
#define __ENGINEER_UI_H

void initializeScreen(int numEngineer);

void updateScreenDistToNext(int numEngineer, int distToNext);

void updateScreenNewLandmark(int numEngineer,
                             const char *nextNode,
                             const char *prevNode,
                             int distToNext);

void updateScreenNewSensor(int numEngineer,
                           const char *nextNode,
                           const char *prevNode,
                           int distToNext,
                           const char *prevSensor,
                           int expectedTime,
                           int actualTime,
                           int error);

#endif
