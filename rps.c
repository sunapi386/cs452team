#include <bwio.h>
#include <utils.h>
#include <syscall.h>
#include <nameserver.h>

#define _LOG(...) bwprintf(COM2, "[rps] " __VA_ARGS__)


enum {
    SIGN_UP,
    QUIT,
    PLAY_ROCK,
    PLAY_PAPER,
    PLAY_SCISSOR,
};

// http://stackoverflow.com/questions/1640258/need-a-fast-random-generator-for-c
static unsigned long x=123456789, y=362436069, z=521288629;
unsigned long xorshf96(void) {          //period 2^96-1
    unsigned long t;
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;

    t = x;
    x = y;
    y = z;
    z = t ^ x ^ y;

    return z;
}


static void rpsServer() {
    // register to handle client requests: signup, play, quit
    int ret = RegisterAs("rpsServer");
    if(ret < 0) {
        _LOG("Failed to register with nameserver");
        Exit();
    }
    int server_id = MyTid();
    _LOG("Server %d registered", server_id);
    int sender_tid, request_id, recv_ret;
    for(;;) {
        recv_ret = Receive(&sender_tid, &request_id, sizeof(int));
        if(recv_ret != sizeof(int)) {
            _LOG("Server received %d (expected %d)", recv_ret, sizeof(int));
            continue;
        }

        switch(request_id) {
            case SIGN_UP:
            case QUIT:
            case PLAY_ROCK:
            case PLAY_PAPER:
            case PLAY_SCISSOR:
            default:
                _LOG("Bad request number %d", request_id);
        } // switch
    } // for
}

static void rpsClient() {
    // find the rps by querying nameserver
    int server_id = WhoIs("rpsServer");
    if(server_id < 0) {
        _LOG("Invalid WhoIs lookup on rpsServer %d", server_id);
        Exit();
    }
    int client_id = MyTid();
    int request_id;

    // perform set of requests that test the rps server: play 3 rounds of rps
    for(int i = 0; i < 3; i++) {
        request_id = SIGN_UP;

        // _Log("Client %d", client_id);

    }


    // send quit request when finished playing
    // exit gracefully
}

static void rpsMakeClients() { // creates players

}

void rpsUserTask() {
    Create(1, nameserverTask);
    Create(2, rpsServer);
    Create(3, rpsMakeClients);
    Exit();
}
