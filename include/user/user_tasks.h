#ifndef __USER_TASK_H
#define __USER_TASK_H

void userTaskMessage();
void userTaskRip();
void userTaskK3();
void userTaskIdle();

void hwiTesterTask();
void clockServerTesterTask();
void undefinedInstructionTesterTask();
void drawIdle(unsigned int diff);

#endif
