#include <bwio.h>
#include <utils.h>
#include <syscall.h>
#include <nameserver.h>

#define RS_NUM_PLAYERS 4
#define _LOG(...) bwprintf(COM2, "[rps] " __VA_ARGS__)

typedef enum {
    PLAY_ROCK = 0,
    PLAY_PAPER = 1,
    PLAY_SCISSOR = 2,
    SIGN_UP,
    QUIT,
    BEGIN_MATCH,
    OPPONENT_QUIT,
    WIN,
    LOSE,
    DRAW,
} rpsCodes;

// http://stackoverflow.com/questions/1640258/need-a-fast-random-generator-for-c
static unsigned long seed_x, seed_y, seed_z;
unsigned long xorshf96(void) {          //period 2^96-1
    unsigned long t;
    seed_x ^= seed_x << 16;
    seed_x ^= seed_x >> 5;
    seed_x ^= seed_x << 1;

    t = seed_x;
    seed_x = seed_y;
    seed_y = seed_z;
    seed_z = t ^ seed_x ^ seed_y;

    return seed_z;
}

unsigned int rand(unsigned int max) {
    return ((unsigned int)xorshf96() % max);
}


static void rpsServer() {
    // register to handle client requests: signup, play, quit
    int ret = RegisterAs("rpsServer");
    if(ret < 0) {
        _LOG("Failed to register with nameserver");
        Exit();
    }
    int server_tid = MyTid();
    _LOG("Server %d registered", server_tid);
    int sender_tid, request;
    int opponent_tid = 0;

    for(;;) {
        ret = Receive(&sender_tid, &request, sizeof(int));
        if(ret != sizeof(int)) {
            _LOG("Server received %d (expected %d)", ret, sizeof(int));
            continue;
        }

        switch(request) {
            case SIGN_UP:
                _LOG("Signup request by %d\r\n", sender_tid);
                if(opponent_tid) { // either there is opponent
                    _LOG("Match commencing: %d vs %d\r\n");

                    // play game
                    opponent_tid = 0;
                }
                else { // or there is no opponent and we should wait
                    _LOG("No opponent...\r\n");
                    opponent_tid = sender_tid;
                } // else
                break; // SIGN_UP

            case QUIT:
                _LOG("Quit request from %d\r\n", sender_tid);

                break; // QUIT

            case PLAY_ROCK:
            case PLAY_PAPER:
            case PLAY_SCISSOR:
                _LOG("Play request from %d\r\n");

            break;
            default:
                _LOG("Bad request (%d) from %d\r\n", request, sender_tid);
        } // switch
    } // for


}

static void rpsClient() {
    // find the rps by querying nameserver
    int server_tid = WhoIs("rpsServer");
    if(server_tid < 0) {
        _LOG("Invalid WhoIs lookup on rpsServer %d", server_tid);
        Exit();
    }
    int my_tid = MyTid();
    int request;
    int response;
    int ret;

    // perform requests to test the rps server: play 0 < n < 5 games of rps
    for(int games_to_play = rand(5) ; games_to_play >= 0; ) {
        // we have no opponent
        request = SIGN_UP;
        ret = Send(server_tid, (void *)&request, sizeof(int), (void *)&response, sizeof(int));
        if(ret != sizeof(int)) {
            _LOG("Client %d got bad response for signup\r\n", my_tid);
            Exit();
        }
        if(response != BEGIN_MATCH) {
            _LOG("Client %d expected BEGIN_MATCH, got %d\r\n", my_tid, response);
            Exit();
        }

        for(;games_to_play >= 0; games_to_play--) {
        // plays until we end are done or opponent leaves
            _LOG("Client %d has %d games left to play\r\n", my_tid, games_to_play);
            // Play the game
            int move = rand(3);
            char *g_moves[3] = {"Rock", "Paper", "Scissor"};
            _LOG("Client %d will play %s\r\n", my_tid, g_moves[move]);
            ret = Send(server_tid, (void *)&move, sizeof(int), (void *)&response, sizeof(int));
            if(ret != sizeof(int)) {
                _LOG("Client %d got bad response %d from play\r\n", my_tid, ret);
                Exit();
            }
            if(response == OPPONENT_QUIT) {
                _LOG("Client %d's opponent quit, will sign up again\r\n", my_tid);
                break;
            }
            if(response != WIN && response != LOSE && response != DRAW) {
                _LOG("Client %d expected win/loss/draw but got %d\r\n", my_tid, response);
                Exit();
            }
            char *g_result[3] = {"Win", "Lose", "Draw"};
            _LOG("Client %d had a %s\r\n", my_tid, g_result[response]);
        } // for
    } // for

    // send quit request when finished playing
    request = QUIT;
    _LOG("Client %d quitting.\r\n", my_tid);
    ret = Send(server_tid, (void *)&request, sizeof(int), &response, sizeof(int));
    if(ret != sizeof(int)) {
        _LOG("Client %d got a bad response when quitting\r\n", my_tid);
    }
    // exit gracefully
    Exit();
}

static void rpsMakeClients() { // creates players
    for(int i = 0; i < RS_NUM_PLAYERS; i++) {
        int ret = Create(4, rpsClient);
        if(ret < 0) {
            _LOG("Making player %d failed\r\n", i);
            break;
        }
        _LOG("Created player %d tid %d\r\n", i, ret);
    }
    Exit();
}

void rpsUserTask() {
    seed_x = 34589034;  // keyboard mashed seed
    seed_y = 98745372;
    seed_z = 32894984;
    Create(1, nameserverTask);
    Create(2, rpsServer);
    Create(3, rpsMakeClients);
    Exit();
}
