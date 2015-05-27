#include <nameserver.h>
#include <task.h>
#include <bwio.h>
#include <utils.h>

#define NS_PRIORITY 1 // nameserver should have an appropriate priority
#define NS_TID  makeTid(1, NS_PRIORITY, 1)

#define NS_MAX_NAME 16
#define NS_MAX_REGIST_SIZE 256

enum {
    SUCCESS = 0,
    ERROR = -1,
    FULL = -2,
    NO_NAMESERVER = -3,
    NO_TASK = -4,
};

typedef struct {
    char name[NS_MAX_NAME];
    enum type { REGISTER_AS, WHO_IS };
} NSRequest;


void nameserverTask() {
    typedef struct {
        char name[NS_MAX_NAME];
        int tid;
    } registrations[NS_MAX_REGIST_SIZE];

    int num_registered = 0;
    int sender_tid;
    int reply;
    NSRequest request;

    for(;;) {
        int recv_size = Receive(&sender_tid, &request, sizeof(NSRequest));
        if(recv_size != sizeof(NSRequest)) {
            Reply(sender_tid, &(ERROR), sizeof(int));
            continue;
        }
        switch (request.type) {
            case REGISTER_AS:
                if(num_registered > NS_MAX_REGIST_SIZE) {
                    Reply(sender_tid, &(FULL), sizeof(int));
                    break;
                }
                int i;
                for(i = 0; i < num_registered; i++) {
                    if(strcmp(request.name, registrations[i].name) == 0) {
                        break; // and overwrite name at i
                    }
                }
                if(i == num_registered) { // create an entry
                    num_registered++;
                    strncpy( registrations[i].name, request.name, NS_MAX_NAME );
                }
                registrations[i].tid = sender_tid;
                Reply(sender_tid, &(SUCCESS), sizeof(int));

                break; // REGISTER_AS

            case WHO_IS:
                for(int i = 0; i < num_registered; i++) {
                    if(strcmp(registrations[i].name, request.name) == 0) {
                        Reply(sender_tid, &(registrations[i].tid), sizeof(int));
                        break; // resolved whois name to a tid
                    }
                    // no registered tid for that name
                    Reply(sender_tid, &(NO_TASK), sizeof(int));

                }

                break; // WHO_IS
            default:
                Reply( sender_tid, &(ERROR), sizeof(int) );
        } // switch
    } // for
}

int RegisterAs(char *name) {
    NSRequest request;
    request.type = REGISTER_AS;
    strncpy(request.name, name, NS_MAX_NAME);
    int reply;
    int ret = Send( NS_TID, &request, sizeof(NSRequest), &reply, sizeof(int));
    if(ret == sizeof(int)) {
        return reply;
    }
    // if -2 then task id is not an existing task
    return ret == sizeof(int) ? reply : (ret == -2 ? NO_NAMESERVER : ERROR);
}

int WhoIs(char *name) {
    NSRequest request;
    request.type = WHO_IS;
    strncpy(request.name, name, NS_MAX_NAME);
    int reply;
    int bytes = Send( NS_TID, &request, sizeof(NSRequest), &reply, sizeof(int));
    // if -2 then task id is not an existing task
    return ret == sizeof(int) ? reply : (ret == -2 ? NO_NAMESERVER : ERROR);
}

