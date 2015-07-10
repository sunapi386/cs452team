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
    initIOServers();
    initClockDrawer();
    // initLogger();
    initTrackServer();
    initTrain();
    initTurnout();
    initParser();
    initSensor();
    // now created in parser //initEngineer();
    initIdleTask();
    Exit();
}

void shutdownTask() {
    exitIdleTask();
    exitSensor();
    // parser does not need exit
    exitTurnout();
    exitTrain();
    exitTrackServer();
    exitClockDrawer();
    exitIOServers();
    exitClockserver();
    exitNameserver();
}
