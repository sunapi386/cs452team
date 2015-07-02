#include <user/parser.h>
#include <priority.h>
#include <utils.h>
#include <user/vt100.h>
#include <user/syscall.h> // Create
#include <user/turnout.h> // to handle sw
#include <user/train.h> // to handle sw
#include <kernel/task.h> // to call taskDisplayAll()
#include <user/sensor.h> // drawTrackLayoutGraph
#include <user/engineer.h> // createEngineer
#include <debug.h>

// Parser is a giant state machine
// tr train_number train_speed
// rv train_number
// sw turnout_number turnout_direction
// q
#include <user/sensor.h> // SensorHalt

static char *help_message =
"------------------ Trains commands\r\n"
"tr train_num speed                 | set train speed\r\n"
"rv train_num                       | reverse train\r\n"
"sw train_num direction             | switch a turnout\r\n"
"e train_num                        | create engineer for train\r\n"
"h train_num group_char sensor_num  | halts on sensor \r\n"
"------------------ Misc. commands\r\n"
"q                                  | quit\r\n"
"o                                  | redraw turnouts\r\n"
"p                                  | redraw track\r\n"
"d                                  | debug tasks\r\n"
"?                                  | prints this help message\r\n";


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
        O,          // o for printing the turnouts
        DB_TASK,    // db
        E,          // e train_number to spawning an engineer to train
        E_space_1,
        E_train_number,
        HELP,       // ? for printing all available commands
    } state;

    // store input data (train number and speed) until ready to use
    union Data {
        struct Speed { // TR commands
            int train_number;
            int train_speed;
        } speed;
        struct Turnout { // SW commands
            int turnout_number;
            char direction;
        } turnout;
        struct Reverse { // RV data
            int train_number;
        } reverse;
        struct SensorHalt {
            int train_number;
            char sensor_group;
            int sensor_number;
        } sensor_halt;
        struct Engineer {
            int train_number;
        } engineer;
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
                    case 'o': p->state = O; break;
                    case 'e': p->state = E; break;
                    case '?': p->state = HELP; break;
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
                if('a' <= c && c <= 'e') {
                    p->data.sensor_halt.sensor_group = c;
                    p->state = H_sensor_group;
                }
                else {
                    p->state = Error;
                }
                break;
            }
            case H_sensor_group: {
                REQUIRE(' ', H_space_3);
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
                p->data.turnout.turnout_number = 0;
                p->state =  append_number(c, &(p->data.turnout.turnout_number)) ?
                            SW_turnout_number :
                            Error;
                break;
            }
            case SW_turnout_number: {
                if(! append_number(c, &(p->data.turnout.turnout_number))) {
                    REQUIRE(' ', SW_space_2);
                }
                break;
            }
            case SW_space_2: {
                switch(c) { // either 'c' or 's'
                case 'c':
                    p->data.turnout.direction = 'c';
                    p->state = SW_turnout_dir;
                    break;
                case 's':
                    p->data.turnout.direction = 's';
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

            // ----------- e for engineer ----------- //
            case E: {
                REQUIRE(' ', E_space_1);
                break;
            }
            case E_space_1: {
                p->data.engineer.train_number = 0;
                p->state =  append_number(c, &(p->data.engineer.train_number)) ?
                            E_train_number :
                            Error;
            }
            case E_train_number: {
                if(! append_number(c, &(p->data.engineer.train_number))) {
                    p->state = Error;
                }
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
        sputstr(&disp_msg, "  | ");
        // should be on an end state
        switch(p->state) {
            case H_sensor_number: {
                int train_number = p->data.sensor_halt.train_number;
                int sensor_group = p->data.sensor_halt.sensor_group;
                int sensor_number = p->data.sensor_halt.sensor_number;
                // check each is valid
                if(train_number < 0 || 80 < train_number) {
                    sputstr(&disp_msg,
                        "H: bad train_number expects 0 to 80\r\n");
                }
                else if(sensor_group < 'a' || 'e' < sensor_group) {
                    sputstr(&disp_msg,
                        "H: bad sensor_group char, expects 'a' to 'e'\r\n");
                }
                else if(sensor_number < 1 || 16 < sensor_number) {
                    sputstr(&disp_msg,
                        "H: bad sensor_number, expects 1 to 16\r\n");
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
                    sputstr(&disp_msg,"setting train speed\r\n");
                    trainSetSpeed(train_number, train_speed);
                }
                else {
                    sputstr(&disp_msg, "TR: bad train_number or train_speed\r\n");
                }

                break;
            }
            case RV_train_number: {
                // check train_number
                int train_number = p->data.reverse.train_number;
                if(0 <= train_number && train_number <= 80) {
                    sputstr(&disp_msg,"Reversing\r\n");
                    trainSetReverseNicely(train_number);
                }
                else {
                    sputstr(&disp_msg, "RV: bad train_number\r\n");
                }

                break;
            }
            case SW_turnout_dir: {
                // check turnout_number
                int turnout_number = p->data.turnout.turnout_number;
                if(   (1 <= turnout_number && turnout_number <= 18) ||
                    (153 <= turnout_number && turnout_number <= 156)) {
                    char dirctn = p->data.turnout.direction;
                    assert(dirctn == 's' || dirctn == 'c');
                    sputstr(&disp_msg, "turnout changing\r\n");
                    turnoutSet(turnout_number, dirctn);
                }
                else {
                    sputstr(&disp_msg, "SW: bad turnout_number\r\n");
                }

                break;
            }
            case Q_Q: {
                sputstr(&disp_msg, "Quit\r\n");
                run = false;
                break;
            }
            case DB_TASK: {
                sputstr(&disp_msg, "Debug task!\r\n");
                taskDisplayAll();
                break;
            }
            case P: {
                sputstr(&disp_msg, "Drawing track!\r\n");
                drawTrackLayoutGraph(A);
                break;
            }
            case O: {
                sputstr(&disp_msg, "Drawing turnouts!\r\n");
                printResetTurnouts();
                break;
            }
            case E_train_number: {
                int train_number = p->data.engineer.train_number;
                if(0 <= train_number && train_number <= 80) {
                    sputstr(&disp_msg, "Create engineer for ");
                    sputint(&disp_msg, train_number, 10);
                    sputstr(&disp_msg, "\r\n");
                    engineerCreate(train_number);
                }
                else {
                    sputstr(&disp_msg, "Error bad train_number ");
                    sputint(&disp_msg, train_number, 10);
                    sputstr(&disp_msg, "\r\n");
                }
                break;
            }
            case HELP: {
                sputstr(&disp_msg, help_message);
                break;
            }
            default: {
                sputstr(&disp_msg, "Command not recognized!\r\n");
            }
        } // switch
        if(run) {
            sputstr(&disp_msg, "> ");
        }
        p->state = Empty;
    } // else if carriage return
    PutString(COM2, &disp_msg);
    return run;
}


void parserTask() {
    Parser p;
    p.state = Empty;
    bool run = true; // set to false when we detect quit command

    // draw the parsing window, etc
    String s;
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

    // write output
    Halt();
}

void initParser() {
    Create(PRIORITY_PARSER, parserTask);
}
