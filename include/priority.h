#ifndef PRIORITY_H
#define PRIORITY_H

// Common system level priorities
#define PRIORITY_INIT               0
#define PRIORITY_NOTIFIER           1
#define PRIORITY_CLOCK_SERVER       2
#define PRIORITY_IRQ_SERVER         3
#define PRIORITY_SERVER             4
#define PRIORITY_USERTASK           5
#define PRIORITY_IDLE               6

// Input/output
#define PRIORITY_TRAIN_IN_NOTIFIER     PRIORITY_NOTIFIER
#define PRIORITY_TRAIN_OUT_NOTIFIER    PRIORITY_NOTIFIER
#define PRIORITY_MONITOR_IN_NOTIFIER   PRIORITY_NOTIFIER
#define PRIORITY_MONITOR_OUT_NOTIFIER  PRIORITY_NOTIFIER
#define PRIORITY_CLOCK_NOTIFIER        PRIORITY_NOTIFIER

#define PRIORITY_TRAIN_IN_SERVER       PRIORITY_IRQ_SERVER
#define PRIORITY_TRAIN_OUT_SERVER      PRIORITY_IRQ_SERVER
#define PRIORITY_MONITOR_IN_SERVER     PRIORITY_IRQ_SERVER
#define PRIORITY_MONITOR_OUT_SERVER    PRIORITY_IRQ_SERVER

#define PRIORITY_NAMESERVER            PRIORITY_SERVER
#define PRIORITY_LOG_SERVER            PRIORITY_SERVER

#define PRIORITY_CLOCK_DRAWER          PRIORITY_USERTASK
#define PRIORITY_PARSER                PRIORITY_USERTASK
#define PRIORITY_SENSOR_TASK           PRIORITY_USERTASK

#endif
