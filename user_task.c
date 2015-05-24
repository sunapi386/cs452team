#include "user_task.h"

#define COM2 1

void userModeTask(){
    while(1) {
      bwputstr(COM2, "-> -> USER TASK!\r\n");
      asm volatile("swi 0");
    }
}
