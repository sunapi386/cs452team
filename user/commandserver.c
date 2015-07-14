#include <debug.h>
#include <utils.h>
#include <user/syscall.h>
#include <user/train.h>
#include <user/commandserver.h>

#define COMMAND_WORKER 34
#define REVERSE_WORKER 35

void commandCopy(Command *dst, const Command *src)
{
    dst->type = src->type;
    dst->trainSpeed = src->trainSpeed;
    dst->trainNumber = src->trainNumber;
    dst->turnoutNumber = src->turnoutNumber;
}

typedef struct {
    size_t head, tail, size;
    Command *buffer;
} CommandQueue;

// Initialize the queue
void initCommandQueue(CommandQueue *q, size_t size, Command *buffer)
{
    q->head = 0;
    q->tail = 0;
    q->size = size;
    q->buffer = buffer;
}

// Put the command in the front of the queue
int enqueueCommand(CommandQueue *q, Command *in)
{
    int ret = 0;
    q->tail = (q->tail + 1) % q->size;
    if (q->tail == q->head)
    {
        // overflow
        q->tail = (q->tail + 1) % q->size;
        ret = -1;
    }
    commandCopy(&(q->buffer[q->tail]), in);
    return ret;
}

// Dequeue the command at the front of the queue, or NULL if empty
int dequeueCommand(CommandQueue *q, Command *out)
{
    // underflow
    if (q->head == q->tail) return -1;
    q->head = (q->head + 1) % q->size;
    commandCopy(out, &(q->buffer[q->head]));
    return 0;
}

// Gets the first command in the queue but does not dequeue it
int peekCommand(CommandQueue *q, Command *out)
{
    // underflow
    if (q->head == q->tail) return -1;
    int headIndex = (q->head + 1) % q->size;
    commandCopy(out, &(q->buffer[headIndex]));
    return 0;
}

int isCommandQueueEmpty(CommandQueue *q)
{
    return q->head == q->tail;
}

void commandWorker()
{
    int pid = MyParentTid();

    Command command;
    command.type = COMMAND_WORKER;

    for (;;)
    {
        int len = Send(pid, &command, sizeof(command), &command, sizeof(command));
        assert(len == sizeof(Command));

        switch(command.type)
        {
        case COMMAND_SET_SPEED:
            trainSetSpeed(command.trainNumber, command.trainSpeed);
            break;
        case COMMAND_SET_TURNOUT:

        case COMMAND_REVERSE:
        default:
            printf(COM2, "[commandWorker] Invalid command\n\r");
            assert(0);
        }
    }
}

void reverseWorker()
{
    for (;;)
    {
        // send to commandServer: await reverse command

        // send to commandServer: about to set speed 0
        // set stop
        // delay

        // send to commandServer: about to reverse
        // set reverse
        // delay

        // send to commandServer: about to set original speed
        // set speed
    }
}

void commandServer()
{
    int tid = 0;
    Command cmd;
    for (;;)
    {
        int len = Receive(&tid, &cmd, sizeof(cmd));
        assert(len == sizeof(Command));

        switch (cmd.type) {

        /*
            Producers
        */
        case COMMAND_SET_SPEED:
        {
            int speed = cmd.trainSpeed;
            int trainNumber = cmd.trainNumber;
            // put into the commandQueue

            // if commandWorkerTid is set
            //      unblock it
            // else
            //      put it into queue

            break;
        }
        case COMMAND_REVERSE:
        {
            // put into the commandQueue
        }
        case COMMAND_SET_TURNOUT:
        {
            // put into the commandQueue
            break;
        }

        /*
            Consumers
        */
        case COMMAND_COURIER:
        {
            // write down id (block it)
            break;
        }

        case COMMAND_WORKER:
        {
            // if commandQueue is empty:
            //      write down id
            // else
            //      dequeue
            //      reply to worker
            break;
        }

        case REVERSE_WORKER:
        {
            // if commandQueue is empty

        }

        default:
            printf(COM2, "[commandServer] Invalid command\n\r");
            assert(0);
        }
    }
}

