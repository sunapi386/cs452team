#ifndef __USER_TASK_H
#define __USER_TASK_H

void userTaskMessage();
void userTaskRip();
void initIdleTask();
void exitIdleTask();

void userTaskHwiTester();
void clockServerTesterTask();
void undefinedInstructionTesterTask();

void clockWaiterTask(int task_to_notify);
#endif
