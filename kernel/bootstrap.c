#include <kernel/bootstrap.h>
#include <priority.h>
#include <user/clock_drawer.h>
#include <user/clockserver.h>
#include <user/io.h>
#include <user/logger.h>
#include <user/nameserver.h>
#include <user/parser.h>
#include <user/sensor.h>
#include <user/syscall.h>
#include <user/trackserver.h>
#include <user/train.h>
#include <user/turnout.h>
#include <user/user_tasks.h>

void bootstrapTask() {
    initNameserver();
    initClockserver();

    // Create IO Servers
    Create(PRIORITY_TRAIN_OUT_SERVER, trainOutServer);
    Create(PRIORITY_TRAIN_IN_SERVER, trainInServer);
    Create(PRIORITY_MONITOR_OUT_SERVER, monitorOutServer);
    Create(PRIORITY_MONITOR_IN_SERVER, monitorInServer);

    // Create user task
    Create(PRIORITY_CLOCK_DRAWER, clockDrawer);
    // initLogger();
    initTrackServer();
    initTrain();
    initTurnout();
    initParser();
    initSensor();

    // now created in parser //initEngineer();
    // Create idle task
    Create(PRIORITY_IDLE, idleProfiler);

    Exit();
}

void shutdownTasks() {
    exitClockserver();
    exitNameserver();
}
