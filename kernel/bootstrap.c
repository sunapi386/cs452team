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
#include <user/track_controller.h>
#include <user/train.h>
#include <user/turnout.h>
#include <user/user_tasks.h>


void bootstrapTask() {
    // Create name server
    Create(PRIORITY_NAMESERVER, nameserverTask);

    // Create clock server
    Create(PRIORITY_CLOCK_SERVER, clockServerTask);

    // Create IO Servers
    Create(PRIORITY_TRAIN_OUT_SERVER, trainOutServer);
    Create(PRIORITY_TRAIN_IN_SERVER, trainInServer);
    Create(PRIORITY_MONITOR_OUT_SERVER, monitorOutServer);
    Create(PRIORITY_MONITOR_IN_SERVER, monitorInServer);

    // Create user task
    Create(PRIORITY_CLOCK_DRAWER, clockDrawer);
    // initLogger();
    initController();
    initTrain();
    initTurnout();
    initParser();
    initSensor();
    // Create idle task
    Create(PRIORITY_IDLE, idleProfiler);

    Exit();
}
