#ifndef PRIORITY_H
#define PRIORITY_H

/**
Just a gentle reminder that task priorities moving forwards will be --very--
important for your kernels in terms of correct behaviour.

4 through 12 priority levels are likely not enough for dealing with all of
the OS features in addition to the trains. It should be possible to get 32
priorities working with close to (if not the same) efficiency, especially if
you are using a naive strategy to deal with fewer priorities.

In particular, it is important to use priorities in such a way that you
guarantee the most important interrupts / handling take precedence over
others. For instance, it should not be possible for I/O (in either
direction) to cause clock ticks to be lost.

Ben
*/

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

#define PRIORITY_CLOCK_DRAWER          PRIORITY_USERTASK
#define PRIORITY_PARSER                PRIORITY_USERTASK
#define PRIORITY_SENSOR_TASK           PRIORITY_USERTASK

#endif
