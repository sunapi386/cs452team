#include <utils.h>
#include <user/vt100.h>
#include <user/syscall.h>
#include <user/engineer_ui.h>

static inline int abs(int num) {
    return (num < 0 ? -1 * num : num);
}

#define ENG1_INIT_STR "\033[1;80HNext landmark:     " \
                      "\033[2;80HPrevious landmark: " \
                      "\033[3;80HDistance to next:  " \
                      "\033[4;80HPrevious sensor:   " \
                      "\033[5;80HExpected (ticks):  " \
                      "\033[6;80HActual   (ticks):  " \
                      "\033[7;80HError    (ticks):  "

#define ENG2_INIT_STR "\033[1;120HNext landmark:     " \
                      "\033[2;120HPrevious landmark: " \
                      "\033[3;120HDistance to next:  " \
                      "\033[4;120HPrevious sensor:   " \
                      "\033[5;120HExpected (ticks):  " \
                      "\033[6;120HActual   (ticks):  " \
                      "\033[7;120HError    (ticks):  "

#define ENG1_LANDMARK_STR "\033[1;100H%s    " \
                          "\033[2;100H%s    " \
                          "\033[3;100H%d.%d cm     "

#define ENG2_LANDMARK_STR "\033[1;140H%s    " \
                          "\033[2;140H%s    " \
                          "\033[3;140H%d.%d cm     "

#define ENG1_SENSOR_STR "\033[4;100H%s    " \
                        "\033[5;100H%d    " \
                        "\033[6;100H%d    " \
                        "\033[7;100H%d    "

#define ENG2_SENSOR_STR "\033[4;140H%s    " \
                        "\033[5;140H%d    " \
                        "\033[6;140H%d    " \
                        "\033[7;140H%d    "

void initializeScreen(int numEngineer)
{
    if (numEngineer)
    {
        printf(COM2, VT_CURSOR_SAVE
                     ENG2_INIT_STR
                     VT_CURSOR_RESTORE);
    }
    else
    {
        printf(COM2, VT_CURSOR_SAVE
                     ENG1_INIT_STR
                     VT_CURSOR_RESTORE);
    }
}

void updateScreenDistToNext(int numEngineer, int distToNext)
{
    if (numEngineer)
    {
        printf(COM2, VT_CURSOR_SAVE
                     "\033[3;140H%d.%d cm     "
                     VT_CURSOR_RESTORE,
                     distToNext / 10000,
                     abs(distToNext) % 10000);
    }
    else
    {
        printf(COM2, VT_CURSOR_SAVE
                     "\033[3;100H%d.%d cm     "
                     VT_CURSOR_RESTORE,
                     distToNext / 10000,
                     abs(distToNext) % 10000);

    }
}

void updateScreenNewLandmark(int numEngineer,
                             const char *nextNode,
                             const char *prevNode,
                             int distToNext)
{
    if (numEngineer)
    {
        printf(COM2, VT_CURSOR_SAVE
                     ENG2_LANDMARK_STR
                     VT_CURSOR_RESTORE,
                     nextNode,
                     prevNode,
                     distToNext / 10000,
                     abs(distToNext) % 10000);
    }
    else
    {
        printf(COM2, VT_CURSOR_SAVE
                     ENG1_LANDMARK_STR
                     VT_CURSOR_RESTORE,
                     nextNode,
                     prevNode,
                     distToNext / 10000,
                     abs(distToNext) % 10000);
    }

}

void updateScreenNewSensor(int numEngineer,
                                         const char *nextNode,
                                         const char *prevNode,
                                         int distToNext,
                                         const char *prevSensor,
                                         int expectedTime,
                                         int actualTime,
                                         int error)
{
    if (numEngineer)
    {
        printf(COM2, VT_CURSOR_SAVE
                     ENG2_LANDMARK_STR
                     ENG2_SENSOR_STR
                     VT_CURSOR_RESTORE,
                     nextNode,
                     prevNode,
                     distToNext / 10000,
                     abs(distToNext) % 10000,
                     prevSensor,
                     expectedTime,
                     actualTime,
                     error);
    }
    else
    {
        printf(COM2, VT_CURSOR_SAVE
                     ENG1_LANDMARK_STR
                     ENG1_SENSOR_STR
                     VT_CURSOR_RESTORE,
                     nextNode,
                     prevNode,
                     distToNext / 10000,
                     abs(distToNext) % 10000,
                     prevSensor,
                     expectedTime,
                     actualTime,
                     error);
    }
}
