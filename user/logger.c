#include <user/logger.h>
#include <priority.h>
#include <user/syscall.h> // IPC, Time
#include <debug.h> // assert
#include <utils.h> // String stuff
#include <kernel/task.h> // taskGetName
#include <user/vt100.h> // screen positioning

#define LOG_SIZE    40

// not typedef to avoid polluting of global namespace
struct LogRequest {
    enum {
    Register,
    Log,
    } type;
    String msg;
};

static int server_tid;

static void server() {
    // circular buffer of log messages
    String logs[LOG_SIZE];
    int logs_head = 0;
    int logs_tail = 0;
    for(int i = 0; i < LOG_SIZE; i++) {
        sinit(&(logs[i]));
    }

    int current_row = 0;
    int num_messages = 0;
    int sender_tid;

    struct LogRequest request;
    while(1) {
        Receive(&sender_tid, &request, sizeof(request));
        Reply(sender_tid, 0, 0);

        switch(request.type) {
            case Log: {
                // build the log message
                char *task_name = taskGetName(taskGetTDById(sender_tid));
                String log_message;
                sinit(&log_message);
                sprintf(&log_message, "#%d %s : %s\r\n",
                    num_messages, task_name, &(request.msg));

                // record an entry into logs
                scopystr(&(logs[logs_tail]), &log_message);
                logs_tail = (logs_tail + 1) % LOG_SIZE;
                if(logs_tail == logs_head) {
                    logs_head = (logs_head + 1) % LOG_SIZE;
                }

                // print to screen
                String out;
                sinit(&out);
                vt_pos(&out, VT_LOG_ROW + current_row, VT_LOG_COL);
                sconcat(&out, &log_message);
                PutString(COM2, &out);

                current_row = (current_row + 1) % LOG_SIZE;

                break;
            }
            default:
                assert(0);
        }

    }

}

void initLogger() {
    server_tid = Create(PRIORITY_LOG_SERVER, server);
}

void log(const char *message) {
    struct LogRequest rq;
    rq.type = Log;
    scopy(&(rq.msg), message);
    Send(server_tid, &rq, sizeof(rq), 0, 0);
}

