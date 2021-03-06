#ifndef __UTILS_H
#define __UTILS_H

#define STR_MAX_LEN 1024

typedef struct String {
    unsigned int len;
    char buf[STR_MAX_LEN + 1];
} String;

typedef unsigned int size_t;
typedef enum { false, true } bool;

void memcpy(void *dest, const void *src, size_t n);
void * memset(void *ptr, int value, size_t num);
int strcmp (const char * dst, const char * src);
char * strncpy(char *dst, const char *src, size_t n);
size_t strlen(const char *str); // does not count null terminator

/*
    CBuffer
*/

typedef struct CBuffer {
    char *data;
    size_t size, head, tail;
} CBuffer;

void CBufferInit(CBuffer *b, char * array, size_t size);
int CBufferPush(CBuffer *b, char ch);
char CBufferPop(CBuffer *b);
bool CBufferIsEmpty(const CBuffer *b);
void CBufferClean(CBuffer *b);
int CBufferPushStr(CBuffer *b, const char *str);
int CBufferPushString(CBuffer *b, const String *s);

/*
    Command & CommandQueue
*/

#define COMMAND_SET_SPEED   30
#define COMMAND_REVERSE     31

typedef struct {
    char type;
    char trainSpeed;
    char trainNumber;
} Command;

typedef struct {
    size_t head, tail, size;
    Command *buffer;
} CommandQueue;

int enqueueCommand(CommandQueue *q, Command *in);
int dequeueCommand(CommandQueue *q, Command *out);
int isCommandQueueEmpty(CommandQueue *q);
void initCommandQueue(CommandQueue *q, size_t size, Command *buffer);

/*
    Sensor & SensorQueue
*/

#define SENSOR_TRIGGER 40
#define SENSOR_TIMEOUT 41

typedef struct {
    int tid, type, nodeIndex, timestamp;
} SensorDelivery;

typedef struct {
    size_t head, tail, size;
    SensorDelivery *buffer;
} SensorQueue;

void sensorDeliveryCopy(SensorDelivery *dst, const SensorDelivery *src);
void initSensorQueue(SensorQueue *q, size_t size, SensorDelivery *buffer);
int enqueueSensor(SensorQueue *q, int tid, int type, int nodeIndex, int timestamp);
int dequeueSensor(SensorQueue *q, SensorDelivery *out);
int isSensorQueueEmpty(SensorQueue *q);

/*
    String
*/

static inline void sinit(String *s) {
    s->len = 0;
    for(int i = 0; i < STR_MAX_LEN; i++) {
        s->buf[i] = '\0';
    }
}
static inline unsigned int slen(String *s) {
    return s->len;
}
void scopy(String *dst, const char *src);
void scopystr(String *dst, String *src);
void sputc(String *s, const char c);
char spop(String *s);
void sputstr(String *s, const char *str);
void sconcat(String *dst, String *src);
void sputuint(String *s, int num, unsigned short base);
void sputint(String *s, int num, unsigned short base);
void sformat(String *s, char *fmt, char * va);
void sprintf(String *s, char *fmt, ... );
void printf(int channel, char *fmt, ... );

#endif
