/** scheduler.h
Round-robin policy.
The highest-priority real-time thread at the front of the FIFO queue will be granted the CPU until it terminates or blocks.

The dispatcher uses a 32-level priority scheme to determine the order of thread execution. Priorities are divided into two classes. The variable class contains threads having priorities from 1 to 15, and the real-time class contains threads with priorities ranging from 16 to 31. (There is also a thread running at priority 0 that is used for memory management.) The dispatcher uses a queue for each scheduling priority and traverses the set of queues from highest to lowest until it finds a thread that is ready to run. If no ready thread is found, the dispatcher will execute a special thread called the idle thread.
*/

#include <task.h>
/**
Keep multiple queues different priorities.
Use a bitmask to keep track which queue contains tasks we should run.
*/