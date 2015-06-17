#include <utils.h>
#include <priority.h>
#include <user/vt100.h>
#include <user/sensor.h>
#include <user/syscall.h>

#define QUERY 133

void intToString(unsigned int number, char *buf)
{
    unsigned int n = 10, len = 1;
    for (;;)
    {
        if (number < n)
        {
            break;
        }
        else
        {
            ++len;
            n *= 10;
        }
    }

    buf[len] = '\0';
    int i;
    for (i = len; i > 0; i--)
    {
        buf[i-1] = (number % 10) + '0'; // Convert to ascii eq
        number /= 10;
    }
}

/**
processData
    Encode a received sensor data into bytes that represents sensor status,
    then use them to populate a buffer, which is then used to output recently
    fired sensor in a scrolling fashion
*/
static inline void
processData(CBuffer *b, const char replyCount, const char data)
{
    char name = 0, offset = replyCount % 2 == 0 ? 0 : 8;

    if      (replyCount < 2) name = 0;
    else if (replyCount < 4) name = 1;
    else if (replyCount < 6) name = 2;
    else if (replyCount < 8) name = 3;
    else                     name = 4;

    char i, index;
    for (i = 0, index = 8; i < 8; i++, index--)
    {
        if ((1 << i) & data)
        {
            if ((b->tail + 1) % b->size == b->head)
            {
                // prevent wrap around: dequeue size/2 items
                // this is needed for scrolling behavior
                b->head = (b->head + b->size/2) % b->size;
            }
            // index + offset: 00001 - 10000
            CBufferPush(b, (name << 5) | (index + offset));
        }
    }
}

static inline void
putStatus(const char status)
{
    // Decode sensor status
    // first 3 byte: A-E, last 5 bytes: 1-16
    unsigned char name = status >> 5; // get upper 3 bits
    unsigned char index = status & 31; // get lower 5 bits (& 11111)
    char indexStr[3];
    intToString(index, indexStr);

    // Output status
    Putc(COM2, 'A' + name);
    PutStr(indexStr);
}

static inline void
draw(const CBuffer *b)
{
    unsigned int head = b->head;
    unsigned int tail = b->tail;
    size_t size = b->size;
    char *buffer = b->data;

    // Go to the correct cursor position
    PutStr("\33[s");
    int i;
    for (i = 0; i < 11; i++)
    {
        if (tail == head)
        {
            break;
        }
        char *pos;
        switch (i)
        {
        case 0:
            pos = "\33[7;25H\33[K";
            break;
        case 1:
            pos = "\33[8;25H\33[K";
            break;
        case 2:
            pos = "\33[9;25H\33[K";
            break;
        case 3:
            pos = "\33[10;25H\33[K";
            break;
        case 4:
            pos = "\33[11;25H\33[K";
            break;
        case 5:
            pos = "\33[12;25H\33[K";
            break;
        case 6:
            pos = "\33[13;25H\33[K";
            break;
        case 7:
            pos = "\33[14;25H\33[K";
            break;
        case 8:
            pos = "\33[15;25H\33[K";
            break;
        case 9:
            pos = "\33[16;25H\33[K";
            break;
        case 10:
            pos = "\33[17;25H\33[K";
            break;
        default:
            pos = "\33[18;25H\33[K";
            break;
        }
        PutStr(pos);
        putStatus(buffer[tail]);
        tail = tail == 0 ? size - 1 : tail - 1;
    }
    PutStr("\33[u");
}

void sensorTask()
{
    char sb[512];
    CBuffer sensorStatus;
    CBufferInit(&sensorStatus, sb, 512);

    for (;;)
    {
        // Query for data
        Putc(COM1, QUERY);

        // Grab 10 replies
        int i;
        for (i = 0; i < 10; i++)
        {
            char c = Getc(COM1);
            if (c != 0)
            {
                processData(&sensorStatus, i, c);
                draw(&sensorStatus);
            }
        }
    }
}
