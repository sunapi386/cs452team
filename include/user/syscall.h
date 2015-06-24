#ifndef __SYSCALL_H
#define __SYSCALL_H

#include <events.h>

#define COM1 0
#define COM2 1

struct String;

#define SYS_AWAIT_EVENT     1
#define SYS_SEND            2
#define SYS_RECEIVE         3
#define SYS_REPLY           4
#define SYS_CREATE          5
#define SYS_EXIT            6
#define SYS_MY_TID          7
#define SYS_MY_PARENT_TID   8
#define SYS_PASS            9
#define SYS_HALT            10

typedef
struct Syscall {
    unsigned int type, arg1, arg2, arg3, arg4, arg5;
} Syscall;


/**
Name. Create - instantiate a task.
Synopsis. int Create( int priority, void (*code) ( ) )
Description. Create allocates and initializes a task descriptor, using the given
priority, and the given function pointer as a pointer to the entry point of
executable code, essentially a function with no arguments and no return value.
When Create returns the task descriptor has all the state needed to run the task,
the task’s stack has been suitably initialized, and the task has been entered into
its ready queue so that it will run the next time it is scheduled.
Returns.
    tid: positive integer task id of the newly created task. The task id must be
    unique, in the sense that no task has, will have or has had the same task id.
    -1 : error(s) occured
Do rough tests to ensure that the function pointer argument is valid.
*/
int Create( int priority, void (*code) ( ) );

/**
Name. MyTid - find my task id.
Synopsis. int MyTid( )
Description. MyTid returns the task id of the calling task.
Returns.
    tid: the positive integer task id of the task that calls it.
*/
int MyTid( );

/**
Name. MyParentTid - find the task id of the task that created the running
task.
Synopsis. int MyParentTid( )
Description. MyParentTid returns the task id of the task that created the calling task.
This will be problematic only if the task has exited or been destroyed, in which
case the return value is implementation-dependent.
Returns.
    MyParentTidtid: the task id of the task that created the calling task.
    -1 : if the parent has exited, been destroyed, or is in the process of being destroyed
*/
int MyParentTid( );

/**
Name. Pass - cease execution, remaining ready to run.
Synopsis. void Pass( )
Description. Pass causes a task to stop executing. The task is moved to the end
of its priority queue, and will resume executing when next scheduled.
*/
void Pass( );

/**
Name. Exit - terminate execution forever. Synopsis. void Exit( )
Description. Exit causes a task to cease execution permanently. It is removed
from all priority queues, send queues, receive queues and awaitEvent queues.
Resources owned by the task, primarily its memory and task descriptor are not reclaimed.
Returns. Exit does not return. If a point occurs where all tasks have exited the
kernel should return cleanly to RedBoot.
*/
void Exit( );

/**
Send - send a message

Returns: The size of the message supplied by the replying task.
    -1 the task id is impossible.
    -2 the task id is not an existing task.
    -3 the send-receive-reply transaction is incomplete.
 */
int Send( int tid, void *msg, unsigned int msglen, void *reply, unsigned int replylen );

/**
Receive - receive a message

Returns: The size of the message sent.
*/
int Receive( int *tid, void *msg, unsigned int msglen );

/**
Reply - reply to a message

Returns: 0 if the reply succeeds.
    -1 the task id is not a possible task id.
    -2 the task id is not an existing task.
    -3 the task is not reply blocked.
*/
int Reply( int tid, void *reply, unsigned int replylen );

/**
AwaitEvent - block until event with eventType
Description. AwaitEvent blocks until the event identified by eventid occurs then returns.
The following details are implementation-dependent.
• the kernel does not collects volatile data, and does not re-enables the interrupt.
• interrupts are enabled when AwaitEvent returns.
• at most one task to block on a single event.
Returns.
• volatile data – in the form of a positive integer.
• 0 – volatile data is in the event buffer.
• -1 – invalid event.
• -2 – corrupted volatile data. Error indication in the event buffer.
• -3 – volatile data must be collected and interrupts re-enabled in the
caller.
*/
int AwaitEvent(int eventType);

/**
Returns:
0   Success
-1  Name table full
-2  Name server doesn't exist
-3  Generic error
*/
int RegisterAs(char *name);

/**
Returns:
>=0  Task id
-1   No task registered under that name
-2   Name server doesn't exist
-3   Generic error
 */
int WhoIs(char *name);

int Time();

int Delay(int ticks);

int DelayUntil(int ticks);

/**
Getc returns first unreturned character from the given UART.
FIXME: Discuss in documentation: How transmission errors are handled is implementation-dependent.
Getc is actually a wrapper for a send to the serial server.
Returns.
• character – success.
• -1 – if the serial server task id inside the wrapper is invalid.
• -2 – if the serial server task id inside the wrapper is not the serial
*/
int Getc(int channel);
/**
Putc queues the given character for transmission by the given UART.
On return the only guarantee is that the character has been queued.
Whether it has been transmitted or received is not guaranteed.
FIXME: Discuss in documentation: How configuration errors are handled is implementation-dependent.
Putc is actually a wrapper for a send to the serial server.
Returns.
• 0 – success.
• -1 – if the serial server task id inside the wrapper is invalid.
• -2 – if the serial server task id inside the wrapper is not the serial
server.
*/
int Putc(int channel, char c);

int PutStr(int channel, char *str);

int PutString(int channel, struct String *s);

void Halt();

#endif // __SYSCALL_H
