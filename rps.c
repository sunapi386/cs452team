#include <bwio.h>
#include <utils.h>
#include <syscall.h>
#include <nameserver.h>
#include <user_task.h>

#define RS_NUM_PLAYERS 4
#define RS_MAX_PLAYERS 32
#define _LOG(...) bwprintf(COM2, "[rps] " __VA_ARGS__)

typedef enum {
    EMPTY_ENTRY = -1,
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
    SERVER_FULL,
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
    struct { // storage of game session
        int tid; // owner task
        rpsCodes request;
        int opponent_tid;
    } players[RS_MAX_PLAYERS];

    for(int i = 0; i < RS_MAX_PLAYERS; i++) { // init game sessions
        players[i].tid = players[i].opponent_tid = 0;
        players[i].request = EMPTY_ENTRY;
    }

    _LOG("Server: registering with nameserver\r\n");
    // register to handle client requests: signup, play, quit
    int ret = RegisterAs("rpsServer");
    if(ret < 0) {
        _LOG("Server: Failed to register with nameserver\r\n");
        Exit();
    }
    int server_tid = MyTid();
    _LOG("Server %d registered\r\n", server_tid);
    int sender_tid, request, reply;
    int waiting_tid = 0;
    int num_players = 0;

    for(;;) {
        ret = Receive(&sender_tid, &request, sizeof(int));
        if(ret != sizeof(int)) {
            _LOG("Server: received %d (expected size %d)\r\n", ret, sizeof(int));
            continue;
        }
        _LOG("Server received %d %d\r\n", sender_tid, request);

        switch(request) {
            case SIGN_UP:
                _LOG("Server: Signup request by %d\r\n", sender_tid);

                // check if we have space to register player
                if(num_players == RS_MAX_PLAYERS) {
                    _LOG("Server: Too many players, rejecting %d\r\n", sender_tid);
                    reply = SERVER_FULL;
                    Reply(sender_tid, (void *)&reply, sizeof(int));
                    break;
                }
                num_players++;

                _LOG("Server: Adding player %d to players table\r\n", sender_tid);
                // find free entry
                int new_entry;
                for(new_entry = 0; new_entry < RS_MAX_PLAYERS; new_entry++) {
                    if( players[new_entry].tid == 0 &&
                        players[new_entry].opponent_tid == 0
                        && players[new_entry].request == EMPTY_ENTRY ) {
                        break;
                    }
                }
                players[new_entry].tid = sender_tid;
                players[new_entry].request = SIGN_UP;

                if(! waiting_tid) { // there is no opponent
                    _LOG("Server: No opponent, wait...\r\n");
                    waiting_tid = sender_tid;
                }
                else { // or there is opponent to play
                    _LOG("Server: Match commencing: %d vs %d\r\n");
                    // find opponent's entry and link them up
                    for(int op_id = 0; op_id < RS_MAX_PLAYERS; op_id++) {
                        if(players[op_id].tid == waiting_tid) {
                            players[new_entry].opponent_tid = waiting_tid;
                            players[op_id].opponent_tid = sender_tid;
                        }
                    }
                    waiting_tid = 0;
                } // else


                break; // SIGN_UP

            case QUIT:
                _LOG("Server: Quit request from %d\r\n", sender_tid);
                // find the player id
                int game_id;
                for(game_id = 0; game_id < RS_MAX_PLAYERS; game_id++) {
                    if(players[game_id].tid == sender_tid) {
                        break; // found the player in players list
                    }
                }
                if(game_id == RS_MAX_PLAYERS) { // didn't find player
                    _LOG("Server: Got QUIT from player %d but is not in match\r\n", sender_tid);
                    break;
                }
                players[game_id].request = QUIT; // set his status to quit
                int opponent = players[game_id].opponent_tid;
                _LOG("Server: notifying %d's opponent %d that he quit\r\n", sender_tid, opponent);
                reply = QUIT;
                Reply(sender_tid, (void *)&reply, sizeof(int)); // ack
                reply = OPPONENT_QUIT;
                Reply(opponent, (void *)&reply, sizeof(int)); // tell opponent
                // remove player's entry in table
                players[game_id].tid = players[game_id].opponent_tid = 0;
                players[game_id].request = EMPTY_ENTRY;
                num_players--;

                break; // QUIT

            case PLAY_ROCK:
            case PLAY_PAPER:
            case PLAY_SCISSOR:
                _LOG("Server: Play request from %d\r\n");
                int sender_idx;
                // find the sender_idx
                for(sender_idx = 0; sender_idx < RS_MAX_PLAYERS; sender_idx++) {
                    if(players[sender_idx].tid == sender_tid) {
                        break;
                    }
                }
                // check if valid sender_idx
                if(sender_idx == RS_MAX_PLAYERS) {
                    _LOG("Server: Player %d requests play, but not in any match\r\n", sender_tid);
                    break;
                }
                // get opponent_idx
                int opponent_tid = players[sender_idx].opponent_tid;
                int opponent_idx;
                for(opponent_idx = 0; opponent_idx < RS_MAX_PLAYERS; opponent_idx++) {
                    if(players[opponent_idx].tid == opponent_tid) {
                        break;
                    }
                }
                // check if valid opponent_idx
                if(opponent_idx >= RS_MAX_PLAYERS || opponent_idx < 0) {
                    _LOG("Server: Player %d has a bad opponent %d!\r\n", sender_tid, opponent_idx);
                    break;
                }
                players[sender_idx].request = request;


                // check if my opponent played a move yet & determine winner
                if( players[opponent_idx].request == PLAY_ROCK ||
                    players[opponent_idx].request == PLAY_PAPER ||
                    players[opponent_idx].request == PLAY_SCISSOR) {
                // FIXME: ALways replying false, do logic here
                    reply = LOSE;
                    Reply(sender_tid, (void *)&reply, sizeof(int));
                    Reply(opponent_tid, (void *)&reply, sizeof(int));
                    players[sender_idx].request = EMPTY_ENTRY;
                    players[opponent_idx].request = EMPTY_ENTRY;
                    _LOG("Server: Round complete for %d and %d\r\n", sender_tid, opponent_tid);
                    bwgetc(COM2); // pause and wait for TA to see what happened
                }
                else { // opponent hasn't played yet
                    _LOG("Server: Player %d's opponent %d has not played yet\r\n", sender_tid, opponent_tid);
                }

            break;
            default:
                _LOG("Server: Bad request (%d) from %d\r\n", request, sender_tid);
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
    _LOG("\trpsClient %d, WhoIs %d\r\n", my_tid, server_tid);
    int request, response, ret;

    // perform requests to test the rps server: play 0 < n < 5 games of rps
    for(int games_to_play = rand(5) ; games_to_play >= 0; ) {
        // we have no opponent
        request = SIGN_UP;
        ret = Send(server_tid, (void *)&request, sizeof(int), (void *)&response, sizeof(int));
        if(ret != sizeof(int)) {
            _LOG("Client %d got bad response for signup\r\n", my_tid);
            Exit();
        }
        // _LOG("\trpsClient %d server tid %d\r\n", my_tid, server_tid);
        if(response == SERVER_FULL) {
            _LOG("Client %d got SERVER_FULL, trying later\r\n", my_tid);
            games_to_play++;
            continue;
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
