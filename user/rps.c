// #include <bwio.h>
// #include <utils.h>
// #include <user/syscall.h>
// #include <user/nameserver.h>
// #include <user/user_tasks.h>

// #define RS_NUM_PLAYERS 6
// #define RS_MAX_PLAYERS 32
// #define _LOG(...) bwprintf(COM2, "[rps] " __VA_ARGS__)
// #define _LOG_SRV(msg, ...) bwprintf(COM2, "[rps Server %d] " msg, server_tid, ## __VA_ARGS__)
// #define _LOG_CLI(msg, ...) bwprintf(COM2, "[rps Client %d] " msg, my_tid, ## __VA_ARGS__)

// typedef enum {
//     WIN = -3,
//     LOSE = -2,
//     DRAW = -1,
//     PLAY_ROCK = 0,
//     PLAY_PAPER = 1,
//     PLAY_SCISSOR = 2,
//     SIGN_UP,
//     QUIT,
//     BEGIN_MATCH,
//     OPPONENT_QUIT,
//     SERVER_FULL,
//     EMPTY_ENTRY,
// } rpsCodes;

// // http://stackoverflow.com/questions/1640258/need-a-fast-random-generator-for-c
// static unsigned long seed_x, seed_y, seed_z;
// unsigned long xorshf96(void) {          //period 2^96-1
//     unsigned long t;
//     seed_x ^= seed_x << 16;
//     seed_x ^= seed_x >> 5;
//     seed_x ^= seed_x << 1;

//     t = seed_x;
//     seed_x = seed_y;
//     seed_y = seed_z;
//     seed_z = t ^ seed_x ^ seed_y;

//     return seed_z;
// }

// unsigned int rand(unsigned int max) {
//     return ((unsigned int)xorshf96() % max);
// }

// static void rpsServer() {
//     int server_tid = MyTid();
//     struct { // storage of game session
//         int tid; // owner task
//         rpsCodes request;
//         int opponent_tid;
//     } players[RS_MAX_PLAYERS];

//     for(int i = 0; i < RS_MAX_PLAYERS; i++) { // init game sessions
//         players[i].tid = players[i].opponent_tid = 0;
//         players[i].request = EMPTY_ENTRY;
//     }

//     _LOG_SRV("registering with nameserver\r\n");
//     // register to handle client requests: signup, play, quit
//     int ret = RegisterAs("rpsServer");
//     if(ret < 0) {
//         _LOG_SRV("failed to register with nameserver\r\n");
//         Exit();
//     }
//     int sender_tid, request, reply;
//     int waiting_tid = 0;
//     int num_players = 0;

//     for(;;) {
//         ret = Receive(&sender_tid, &request, sizeof(int));
//         if(ret != sizeof(int)) {
//             _LOG_SRV("received %d (expected size %d)\r\n", ret, sizeof(int));
//             continue;
//         }
//         // _LOG_SRV("received player %d request %d\r\n", sender_tid, request);

//         switch(request) {
//             case SIGN_UP:
//                 // _LOG_SRV("signup request by %d\r\n", sender_tid);

//                 // check if we have space to register player
//                 if(num_players == RS_MAX_PLAYERS) {
//                     _LOG_SRV("too many players, rejecting %d\r\n", sender_tid);
//                     reply = SERVER_FULL;
//                     Reply(sender_tid, (void *)&reply, sizeof(int));
//                     break;
//                 }
//                 num_players++;

//                 // _LOG_SRV("adding player %d to players table\r\n", sender_tid);
//                 // find free entry
//                 int new_entry;
//                 for(new_entry = 0; new_entry < RS_MAX_PLAYERS; new_entry++) {
//                     if( players[new_entry].tid == 0 &&
//                         players[new_entry].opponent_tid == 0
//                         && players[new_entry].request == EMPTY_ENTRY ) {
//                         break;
//                     }
//                 }
//                 players[new_entry].tid = sender_tid;
//                 players[new_entry].request = SIGN_UP;

//                 if(! waiting_tid) { // there is no opponent
//                     _LOG_SRV("no opponent, wait...\r\n");
//                     waiting_tid = sender_tid;
//                 }
//                 else { // or there is opponent to play
//                     _LOG_SRV("\t---> MATCH COMMENCING! %d vs %d\r\n", sender_tid, waiting_tid);
//                     // find opponent's entry and link them up
//                     for(int op_id = 0; op_id < RS_MAX_PLAYERS; op_id++) {
//                         if(players[op_id].tid == waiting_tid) {
//                             players[new_entry].opponent_tid = waiting_tid;
//                             players[op_id].opponent_tid = sender_tid;
//                         }
//                     }
//                     reply = BEGIN_MATCH;
//                     Reply(sender_tid, (void *)&reply, sizeof(int));
//                     Reply(waiting_tid, (void *)&reply, sizeof(int));
//                     waiting_tid = 0;
//                 } // else


//                 break; // SIGN_UP

//             case QUIT:
//                 _LOG_SRV("quit request from %d\r\n", sender_tid);
//                 // find the player id
//                 int game_id;
//                 for(game_id = 0; game_id < RS_MAX_PLAYERS; game_id++) {
//                     if(players[game_id].tid == sender_tid) {
//                         break; // found the player in players list
//                     }
//                 }
//                 if(game_id == RS_MAX_PLAYERS) { // didn't find player
//                     _LOG_SRV("got QUIT from player %d but is not in match\r\n", sender_tid);
//                     break;
//                 }
//                 players[game_id].request = QUIT; // set his status to quit
//                 int opponent = players[game_id].opponent_tid;
//                 _LOG_SRV("notifying %d's opponent %d that he quit\r\n", sender_tid, opponent);
//                 reply = QUIT;
//                 Reply(sender_tid, (void *)&reply, sizeof(int)); // ack
//                 reply = OPPONENT_QUIT;
//                 Reply(opponent, (void *)&reply, sizeof(int)); // tell opponent
//                 // remove player's entry in table
//                 players[game_id].tid = players[game_id].opponent_tid = 0;
//                 players[game_id].request = EMPTY_ENTRY;
//                 num_players--;

//                 break; // QUIT

//             case PLAY_ROCK:
//             case PLAY_PAPER:
//             case PLAY_SCISSOR:
//                 _LOG_SRV("play request from %d\r\n", sender_tid);
//                 int sender_idx;
//                 // find the sender_idx
//                 for(sender_idx = 0; sender_idx < RS_MAX_PLAYERS; sender_idx++) {
//                     if(players[sender_idx].tid == sender_tid) {
//                         break;
//                     }
//                 }
//                 // check if valid sender_idx
//                 if(sender_idx == RS_MAX_PLAYERS) {
//                     _LOG_SRV("player %d requests play, but not in any match\r\n", sender_tid);
//                     break;
//                 }
//                 // get opponent_idx
//                 int opponent_tid = players[sender_idx].opponent_tid;
//                 int opponent_idx;
//                 for(opponent_idx = 0; opponent_idx < RS_MAX_PLAYERS; opponent_idx++) {
//                     if(players[opponent_idx].tid == opponent_tid) {
//                         break;
//                     }
//                 }
//                 // check if valid opponent_idx
//                 if(opponent_idx >= RS_MAX_PLAYERS || opponent_idx < 0) {
//                     _LOG_SRV("player %d has a bad opponent %d!\r\n", sender_tid, opponent_idx);
//                     break;
//                 }
//                 players[sender_idx].request = request;


//                 // check if my opponent played a move yet & determine winner
//                 if( players[opponent_idx].request == PLAY_ROCK ||
//                     players[opponent_idx].request == PLAY_PAPER ||
//                     players[opponent_idx].request == PLAY_SCISSOR) {

//                     // RPS logic
//                     int s_play = players[sender_idx].request;
//                     int o_play = players[opponent_idx].request;
//                     if(s_play == o_play) {
//                             reply = DRAW;
//                             Reply(sender_tid, (void *)&reply, sizeof(int));
//                             Reply(opponent_tid, (void *)&reply, sizeof(int));
//                         }
//                     else
//                     if( (s_play == PLAY_PAPER    && o_play == PLAY_ROCK       ) ||
//                         (s_play == PLAY_ROCK     && o_play == PLAY_SCISSOR    ) ||
//                         (s_play == PLAY_SCISSOR  && o_play == PLAY_PAPER      )) {
//                             reply = WIN;
//                             Reply(sender_tid, (void *)&reply, sizeof(int));
//                             reply = LOSE;
//                             Reply(opponent_tid, (void *)&reply, sizeof(int));

//                     }
//                     else {
//                             reply = LOSE;
//                             Reply(sender_tid, (void *)&reply, sizeof(int));
//                             reply = WIN;
//                             Reply(opponent_tid, (void *)&reply, sizeof(int));
//                     }

//                     players[sender_idx].request = EMPTY_ENTRY;
//                     players[opponent_idx].request = EMPTY_ENTRY;
//                     _LOG_SRV("round complete for players %d and %d, press a key...\r\n",
//                         sender_tid, opponent_tid);
//                     bwgetc(COM2); // pause and wait for TA to see what happened
//                 }
//                 else { // opponent hasn't played yet
//                     _LOG_SRV("player %d's opponent %d has not played yet\r\n", sender_tid, opponent_tid);
//                 }

//             break;
//             default:
//                 _LOG_SRV("bad request (%d) from %d\r\n", request, sender_tid);
//         } // switch
//     } // for


// }

// static void rpsClient() {
//     // find the rps by querying nameserver
//     int server_tid = WhoIs("rpsServer");
//     if(server_tid < 0) {
//         _LOG("invalid WhoIs lookup on rpsServer %d", server_tid);
//         Exit();
//     }
//     int my_tid = MyTid();
//     int request, response, ret;
//     // _LOG_CLI("is created, using tid %d as server\n\r", server_tid);
//     // perform requests to test the rps server: play 0 < n < 5 games of rps
//     for(int games_to_play = 3 ; games_to_play > 0; ) {
//         // we have no opponent
//         request = SIGN_UP;
//         ret = Send(server_tid, (void *)&request, sizeof(int), (void *)&response, sizeof(int));
//         if(ret != sizeof(int)) {
//             _LOG_CLI("got bad response for signup\r\n");
//             Exit();
//         }
//         // _LOG("\trpsClient %d server tid %d\r\n", server_tid);
//         if(response == SERVER_FULL) {
//             _LOG_CLI("got SERVER_FULL, trying later\r\n");
//             games_to_play++;
//             continue;
//         }
//         if(response != BEGIN_MATCH) {
//             _LOG_CLI("expected BEGIN_MATCH, got %d\r\n", response);
//             Exit();
//         }

//         for(;games_to_play > 0; games_to_play--) {
//         // plays until we end are done or opponent leaves
//             _LOG_CLI("has %d games left to play\r\n", games_to_play);
//             // Play the game
//             int move = rand(3);
//             static char *g_moves[3] = {"Rock", "Paper", "Scissor"};
//             _LOG_CLI("\t---> WILL PLAY %s\r\n", g_moves[move]);
//             ret = Send(server_tid, (void *)&move, sizeof(int), (void *)&response, sizeof(int));
//             if(ret != sizeof(int)) {
//                 _LOG_CLI("got bad response %d from play\r\n", ret);
//                 Exit();
//             }
//             if(response == OPPONENT_QUIT) {
//                 _LOG_CLI("s opponent quit, will sign up again\r\n");
//                 break;
//             }
//             if(response != WIN && response != LOSE && response != DRAW) {
//                 _LOG_CLI("expected win/loss/draw but got %d\r\n", response);
//                 Exit();
//             }
//             static char *g_result[3] = {"Win", "Lose", "Draw"};
//             _LOG_CLI("\t---> GOT A MATCH RESULT: %s\r\n", g_result[response + 3]);
//         } // for
//     } // for

//     // send quit request when finished playing
//     request = QUIT;
//     _LOG_CLI("has played enough games, bye bye!\r\n");
//     ret = Send(server_tid, (void *)&request, sizeof(int), &response, sizeof(int));
//     if(ret != sizeof(int)) {
//         _LOG_CLI("got a bad response when quitting\r\n");
//     }
//     // exit gracefully
//     Exit();
// }

// static void rpsMakeClients() { // creates players
//     for(int i = 0; i < RS_NUM_PLAYERS; i++) {
//         int ret = Create(2, rpsClient);
//         if(ret < 0) {
//             _LOG("making player %d failed\r\n", i);
//             break;
//         }
//     }
//     Exit();
// }

// void rpsUserTask() {
//     seed_x = 34589034;  // keyboard mashed seed
//     seed_y = 98745372;
//     seed_z = 32894984;
//     Create(1, nameserverTask);
//     Create(2, rpsServer);
//     Create(3, rpsMakeClients);
//     Exit();
// }
