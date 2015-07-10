#include <user/nameserver.h>
#include <kernel/task.h>
#include <bwio.h>
#include <utils.h>
#include <user/syscall.h>
#include <utils.h> // strlen
#include <debug.h> // assert
#include <priority.h>

#define NS_MAX_NAME 32
#define NS_MAX_REGIST_SIZE 32

static const int SUCCESS = 0;
static const int ERROR = -1;
static const int FULL = -2;
static const int NO_NAMESERVER = -3;
static const int NO_TASK = -4;

typedef struct {
    char name[NS_MAX_NAME];
    enum { REGISTER_AS, WHO_IS, EXIT, } type;
} NSRequest;

static int ns_tid;

static void nameserverTask() {
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
            case REGISTER_AS: {

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
            }

            case WHO_IS: {

                bool has_replied = false;

                for(int i = 0; i < num_registered; i++) {
                    if( strcmp(registrations[i].name, request.name) == 0) {
                        Reply(sender_tid, &(registrations[i].tid), sizeof(int));
                        has_replied = true;
                        break; // resolved whois name to a tid
                    }
                }
                // no registered tid for that name
                if(!has_replied) {
                    Reply(sender_tid, (void *)&NO_TASK, sizeof(int));
                }

                break; // WHO_IS
            }

            case EXIT: {
                Reply(sender_tid, 0, 0);
                goto cleanup;
            }

            default:
                debug("nameserver got a bad request from tid %d", sender_tid);
                assert(0);
        } // switch
    } // for

cleanup:
    Exit();
}

int RegisterAs(char *name) {
    assert(strlen(name) < NS_MAX_NAME);
    NSRequest request;
    request.type = REGISTER_AS;
    strncpy(request.name, name, NS_MAX_NAME);
    int reply;
    int ret = Send(ns_tid, &request, sizeof(NSRequest), &reply, sizeof(int));
    // if -2 then task id is not an existing task
    return ret == sizeof(int) ? reply : (ret == -2 ? NO_NAMESERVER : ERROR);
}

int WhoIs(char *name) {
    assert(strlen(name) < NS_MAX_NAME);
    NSRequest request;
    request.type = WHO_IS;
    strncpy(request.name, name, NS_MAX_NAME);
    int reply;
    int ret = Send(ns_tid, &request, sizeof(NSRequest), &reply, sizeof(int));
    // if -2 then task id is not an existing task
    return ret == sizeof(int) ? reply : (ret == -2 ? NO_NAMESERVER : ERROR);
}

void initNameserver() {
    // Create name server
    ns_tid = Create(PRIORITY_NAMESERVER, nameserverTask);
    assert(ns_tid > 0);
}

void exitNameserver() {
    NSRequest request;
    request.type = EXIT;
    Send(ns_tid, &request, sizeof(NSRequest), 0, 0);
}
