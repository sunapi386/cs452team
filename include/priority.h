#ifndef PRIORITY_H
#define PRIORITY_H

// Common system level priorities
#define PRIORITY_INIT               0
#define PRIORITY_NOTIFIER           1
#define PRIORITY_IRQ_SERVER         2
#define PRIORITY_SERVER             8
#define PRIORITY_TASK               16
#define PRIORITY_IDLE               31



// Individual task priorities
#define PRIORITY_SEND_NOTIFIER      PRIORITY_NOTIFIER
#define PRIORITY_RECV_NOTIFIER      PRIORITY_NOTIFIER
#define PRIORITY_TIO_RECV_SERVER    PRIORITY_IRQ_SERVER
#define PRIORITY_TIO_SEND_SERVER    PRIORITY_IRQ_SERVER
#define PRIORITY_MIO_RECV_SERVER    PRIORITY_IRQ_SERVER
#define PRIORITY_MIO_SEND_SERVER    PRIORITY_IRQ_SERVER

#define PRIORITY_CLOCK_NOTIFIER     PRIORITY_NOTIFIER
#define PRIORITY_CLOCK_SERVER       PRIORITY_IRQ_SERVER

#define PRIORITY_NAMESERVER         PRIORITY_SERVER
#define PRIORITY_CLOCK_DRAWER       PRIORITY_TASK
#define PRIORITY_PARSER             PRIORITY_TASK


#endif
