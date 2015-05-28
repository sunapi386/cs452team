#include <nameserver.h>
#include <task.h>
#include <bwio.h>
#include <utils.h>
#include <syscall.h>

#define NS_TID  2

#define NS_MAX_NAME 16
#define NS_MAX_REGIST_SIZE 256

static const int    SUCCESS = 0;
static const int    ERROR = -1;
static const int    FULL = -2;
static const int    NO_NAMESERVER = -3;
static const int    NO_TASK = -4;

typedef struct {
    char name[NS_MAX_NAME];
    enum { REGISTER_AS, WHO_IS, } type;
} NSRequest;


void nameserverTask() {
    bwprintf(COM2, "[nameserverTask] started, myTID %d\r\n", MyTid());
    struct {
        char name[NS_MAX_NAME];
        int tid;
    } registrations[NS_MAX_REGIST_SIZE];

    int num_registered = 0;
    int sender_tid;
    NSRequest request;

    for(;;) {
        int recv_size = Receive(&sender_tid, (void *)&request, sizeof(NSRequest));
        if(recv_size != sizeof(NSRequest)) {
            Reply(sender_tid, (void *)&ERROR, sizeof(int));
            continue;
        }

        switch (request.type) {
            case REGISTER_AS:
                if(num_registered > NS_MAX_REGIST_SIZE) {
                    Reply(sender_tid, (void *)&FULL, sizeof(int));
                    break;
                }
                int i;
                for(i = 0; i < num_registered; i++) {
                    if( strcmp( registrations[i].name, NULL ) == 0) {
                        break; // and overwrite name at i
                    }
                }
                if(i == num_registered) { // create an entry
                    num_registered++;
                    strncpy( registrations[i].name, request.name, NS_MAX_NAME );
                }
                registrations[i].tid = sender_tid;
                Reply(sender_tid, (void *)&SUCCESS, sizeof(int));

                break; // REGISTER_AS

            case WHO_IS:
                for(int i = 0; i < num_registered; i++) {
                    if( strcmp(registrations[i].name, request.name) == 0) {
                        Reply(sender_tid, &(registrations[i].tid), sizeof(int));
                        break; // resolved whois name to a tid
                    }
                    // no registered tid for that name
                    Reply(sender_tid, (void *)&NO_TASK, sizeof(int));
                }

                break; // WHO_IS

            default:
                Reply(sender_tid, (void *)&ERROR, sizeof(int));
        } // switch



    } // for
}

int RegisterAs(char *name) {
    NSRequest request;
    request.type = REGISTER_AS;
    strncpy(request.name, name, NS_MAX_NAME);
    int reply;
    int ret = Send( NS_TID, &request, sizeof(NSRequest), &reply, sizeof(int));
    // if -2 then task id is not an existing task
    return ret == sizeof(int) ? reply : (ret == -2 ? NO_NAMESERVER : ERROR);
}

int WhoIs(char *name) {
    NSRequest request;
    request.type = WHO_IS;
    strncpy(request.name, name, NS_MAX_NAME);
    int reply;
    int ret = Send( NS_TID, &request, sizeof(NSRequest), &reply, sizeof(int));
    // if -2 then task id is not an existing task
    return ret == sizeof(int) ? reply : (ret == -2 ? NO_NAMESERVER : ERROR);
}

