#include <user/parser.h>
#include <priority.h>
#include <utils.h>
#include <user/vt100.h>
#include <user/syscall.h> // Create
#include <user/turnout.h> // to handle sw
#include <user/train.h> // to handle sw
#include <kernel/task.h> // to call taskDisplayAll()
#include <user/sensor.h> // drawTrackLayoutGraph

// Parser is a giant state machine
// tr train_number train_speed
// rv train_number
// sw turnout_number turnout_direction
// q
#include <user/sensor.h> // SensorHalt

typedef struct Parser {
    // state is the last character we've parsed
    enum State {
        Error = -1,
        Empty,
        TR_T,       // tr train_number train_speed
        TR_R,
        TR_space_1,
        TR_train_number,
        TR_space_2,
        TR_speed,
        RV_R,       // rv train_number
        RV_V,
        RV_space,
        RV_train_number,
        SW_S,       // sw turnout_number turnout_direction
        SW_W,
        SW_space_1,
        SW_turnout_number,
        SW_space_2,
        SW_turnout_dir,
        H_H,        // h train_number sensor_group sensor_number
        H_space_1,
        H_train_number,
        H_space_2,
        H_sensor_group,
        H_space_3,
        H_sensor_number,
        Q_Q,        // q
        P,          // p for printing the graph
        DB_TASK,    // db
    } state;

    // store input data (train number and speed) until ready to use
    union Data {
        struct Speed { // TR commands
            int train_number;
            int train_speed;
        } speed;
        struct Junction { // SW commands
            int turnout_number;
            bool curved;
        } junction;
        struct Reverse { // RV data
            int train_number;
        } reverse;
        struct SensorHalt {
            int train_number;
            int sensor_group;
            int sensor_number;
        } sensor_halt;
    } data;

} Parser;

// REQUIRE: require exact character for transition to successor
#define REQUIRE(ch,succ) \
p->state = (c==ch) ? succ : Error;

static inline bool append_number(char c, int *num) {
    if('0' <= c && c <= '9') {
        *num = 10 * (*num) + (int)(c - '0');
        return true;
    }
    return false;
}


static bool parse(Parser *p, char c) {
    // transitions if c is acceptable, else errors
    // http://ascii-table.com/

    bool run = true; // run = false on quit command

    String disp_msg;
    sinit(&disp_msg);


    // only accept ascii characters and newline
    if(' ' <= c && c <= '~') { // if printable
        if('A' <= c && c <= 'Z') {
            c += ('a' - 'A');
        } // lower case any capital character

        switch(p->state) {
            case Empty: {
                switch(c) {
                    case 't': p->state = TR_T; break;
                    case 'r': p->state = RV_R; break;
                    case 's': p->state = SW_S; break;
                    case 'q': p->state = Q_Q; break;
                    case 'd': p->state = DB_TASK; break;
                    case 'h': p->state = H_H; break;
                    case 'p': p->state = P; break;
                    default:  p->state = Error; break;
                }
                break;
            }
            // ----------- h train_number sensor_group sensor_number ------- //
            case H_H: {
                REQUIRE(' ', H_space_1);
                break;
            }
            case H_space_1: {
                p->data.sensor_halt.train_number = 0;
                p->state =  append_number(c, &(p->data.sensor_halt.train_number)) ?
                            H_train_number :
                            Error;
                break;
            }
            case H_train_number: {
                if(! append_number(c, &(p->data.sensor_halt.train_number))) {
                    REQUIRE(' ', H_space_2);
                }
                break;
            }
            case H_space_2: {
                p->data.sensor_halt.sensor_group = 0;
                p->state =  append_number(c, &(p->data.sensor_halt.sensor_group)) ?
                            H_sensor_group :
                            Error;
                break;
            }
            case H_sensor_group: {
                if(! append_number(c, &(p->data.sensor_halt.sensor_group))) {
                    REQUIRE(' ', H_space_3);
                }
                break;
            }
            case H_space_3: {
                p->data.sensor_halt.sensor_number = 0;
                p->state =  append_number(c, &(p->data.sensor_halt.sensor_number)) ?
                            H_sensor_number :
                            Error;
                break;
            }
            case H_sensor_number: {
                if(! append_number(c, &(p->data.sensor_halt.sensor_number))) {
                    p->state = Error;
                }
                break;
            }
            // ----------- tr train_number train_speed ----------- //
            case TR_T: {
                REQUIRE('r', TR_R);
                break;
            }
            case TR_R: {
                REQUIRE(' ', TR_space_1);
                break;
            }
            case TR_space_1: { // parse a train_number
                p->data.speed.train_number = 0;
                p->state =  append_number(c, &(p->data.speed.train_number)) ?
                            TR_train_number :
                            Error;
                break;
            }
            case TR_train_number: {
                if(! append_number(c, &(p->data.speed.train_number))) {
                    REQUIRE(' ', TR_space_2);
                }
                break;
            }
            case TR_space_2: { // parse a train_speed
                p->data.speed.train_speed = 0;
                p->state =  append_number(c, &(p->data.speed.train_speed)) ?
                            TR_speed :
                            Error;
                break;
            }
            case TR_speed: {
                if(! append_number(c, &(p->data.speed.train_speed))) {
                    p->state = Error;
                }
                break;
            } // TR_speed

            // rv train_number ----------- //
            case RV_R: {
                REQUIRE('v', RV_V);
                break;
            }
            case RV_V: {
                REQUIRE(' ', RV_space);
                break;
            }
            case RV_space: { //
                p->data.reverse.train_number = 0;
                p->state =  append_number(c, &(p->data.reverse.train_number)) ?
                            RV_train_number :
                            Error;
                break;
            }
            case RV_train_number: {
                if(! append_number(c, &(p->data.reverse.train_number))) {
                    p->state = Error;
                }
                break;
            } // RV_train_number

            // sw turnout_number turnout_direction ----------- //
            case SW_S: {
                REQUIRE('w', SW_W);
                break;
            }
            case SW_W: {
                REQUIRE(' ', SW_space_1);
                break;
            }
            case SW_space_1: {
                p->data.junction.turnout_number = 0;
                p->state =  append_number(c, &(p->data.junction.turnout_number)) ?
                            SW_turnout_number :
                            Error;
                break;
            }
            case SW_turnout_number: {
                if(! append_number(c, &(p->data.junction.turnout_number))) {
                    REQUIRE(' ', SW_space_2);
                }
                break;
            }
            case SW_space_2: {
                switch(c) { // either 'c' or 's'
                case 'c':
                    p->data.junction.curved = true;
                    p->state = SW_turnout_dir;
                    break;
                case 's':
                    p->data.junction.curved = false;
                    p->state = SW_turnout_dir;
                    break;
                default:
                    p->state = Error;
                }
                break;
            }
            case SW_turnout_dir: {
                p->state = Error;
                break;
            }

            // ----------- q ----------- //
            case Q_Q: {
                p->state = Error;
                break;
            }

            // ---------- print task ---- //
            case DB_TASK: {
                p->state = Error;
                break;
            }
            default:
                break;
        } // switch

        if(p->state == Error) {
            sputstr(&disp_msg, VT_RED);
        }

        sputc(&disp_msg, c);

    } // if printable

    else if(c == VT_CARRIAGE_RETURN) { // user pressed return (enter)
        sputstr(&disp_msg, VT_RESET);
        // should be on an end state
        switch(p->state) {
            case H_sensor_number: {
                int train_number = p->data.sensor_halt.train_number;
                int sensor_group = p->data.sensor_halt.sensor_group;
                int sensor_number = p->data.sensor_halt.sensor_number;
                // check each is valid
                if(train_number < 0 || 80 < train_number) {
                    sprintf(&disp_msg,
                        "H: bad train_number %d, expects 0 to 80\r\n", train_number);
                }
                else if(sensor_group < 'a' || 'e' < sensor_group) {
                    sprintf(&disp_msg,
                        "H: bad sensor_group char, expects 'a' to 'e'\r\n");
                }
                else if(sensor_number < 1 || 16 < sensor_number) {
                    sprintf(&disp_msg,
                        "H: bad sensor_number %d, expects 1 to 16\r\n", sensor_number);
                }
                sensorHalt(train_number, sensor_group, sensor_number);
                break;
            }
            case TR_speed: {
                // check train_number and train_speed
                int train_number = p->data.speed.train_number;
                int train_speed = p->data.speed.train_speed;
                if((0 <= train_number && train_number <= 80) &&
                   (0 <= train_speed  && train_speed  <= 14)) {
                    sprintf(&disp_msg,"TR: %d %d\r\n",
                        train_number, train_speed);
                    trainSetSpeed(train_number, train_speed);
                }
                else {
                    sputstr(&disp_msg,
                        "TR: bad train_number or train_speed\r\n");
                }

                break;
            }
            case RV_train_number: {
                // check train_number
                int train_number = p->data.reverse.train_number;
                if(0 <= train_number && train_number <= 80) {
                    sprintf(&disp_msg,"RV: %d\r\n", train_number);
                    trainSetReverse(train_number);
                }
                else {
                    sputstr(&disp_msg,
                        "RV: bad train_number\r\n");
                }

                break;
            }
            case SW_turnout_dir: {
                // check turnout_number
                int turnout_number = p->data.junction.turnout_number;
                if(   (1 <= turnout_number && turnout_number <= 18) ||
                    (153 <= turnout_number && turnout_number <= 156)) {
                    bool is_curved = p->data.junction.curved;
                    sprintf(&disp_msg, "SW: %d %c\r\n",
                        turnout_number, (is_curved ? 'C' : 'S'));
                    turnoutSet(turnout_number, is_curved);
                }
                else {
                    sputstr(&disp_msg,
                        "SW: bad turnout_number\r\n");
                }

                break;
            }
            case Q_Q: {
                sputstr(&disp_msg, "Quit\r\n");
                run = false;
                Halt();
                break;
            }
            case DB_TASK: {
                taskDisplayAll();
                break;
            }
            case P: {
                drawTrackLayoutGraph(A);
                break;
            }
            default: {
                sputstr(&disp_msg, "Command not recognized!\r\n");
            }
        } // switch
        sputstr(&disp_msg, "> ");
        p->state = Empty;
    } // else if carriage return
    PutString(COM2, &disp_msg);

    return run;
}


void parserTask() {
    String s;
    // sinit(&s);
    // sprintf(&s, "%s%s", VT_CURSOR_SAVE, VT_CURSOR_HIDE);
    // // set scrolling region
    // sprintf(&s, "%s%d;%dr", VT_CSI, 18, 20);
    // sprintf(&s, "%s%s", VT_CURSOR_RESTORE, VT_CURSOR_SHOW);
    // PutString(COM2, &s);

    Parser p;
    p.state = Empty;
    bool run = true; // set to false when we detect quit command

    // draw the parsing window, etc
    sinit(&s);
    vt_pos(&s, VT_PARSER_ROW, VT_PARSER_COL);
    sputstr(&s, "> ");
    PutString(COM2, &s);

    // read input
    while(run) {
        String input;
        sinit(&input);
        while(1) { // read string
            char ch = Getc(COM2);
            sputc(&input, ch);
            if(ch == VT_CARRIAGE_RETURN) break;
        }
        // parse string
        for(unsigned i = 0; i < input.len && run; i++) {
            run = parse(&p, input.buf[i]);
        }
    }
    sinit(&s);
    vt_pos(&s, VT_PARSER_ROW, VT_PARSER_COL);
    sputstr(&s, "Parser exiting");
    PutString(COM2, &s);

    // write output
    Exit();
}

void initParser() {
    Create(PRIORITY_PARSER, parserTask);
}
