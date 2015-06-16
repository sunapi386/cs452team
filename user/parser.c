#include <user/parser.h>
#include <priority.h>
#include <utils.h>
#include <string.h>
#include <user/io.h>
#include <user/vt100.h>
#include <user/syscall.h> // Create

// Parser is a giant state machine
// tr train_number train_speed
// rv train_number
// sw switch_number switch_direction
// q

typedef struct Parser {
    // state is the last character we've parsed
    enum State {
        Empty,      // No input
        Error,      // Any error
        TR_T,       // t
        TR_R,       // r
        TR_space_1,     // first space
        TR_train_number,    // train_number
        TR_space_2,     // second space
        TR_speed,   // train_speed
        RV_R,       // r
        RV_V,       // v
        RV_space,       // space
        RV_train_number, // train_number
        SW_S,       // s
        SW_W,       // w
        SW_space_1, // first space
        SW_switch_number, // train_number
        SW_space_2,     // second space
        SW_switch_dir, // direction is S or C
        Q_Q,      // quit
    } state;

    // store input data (train number and speed) until ready to use
    union Data {
        struct Speed { // TR commands
            int train_number;
            int train_speed;
        } speed;
        struct Junction { // SW commands
            int switch_number;
            bool curved;
        } junction;
        struct Reverse { // RV data
            int train_number;
        } reverse;
    } data;

} Parser;

// REQUIRE: require exact character for transition to successor
#define REQUIRE(ch,succ) \
p->state = (c==ch) ? succ : Error;

bool append_number(char c, int *num) {
    // TODO
    return false;
}


static bool parse(Parser *p, char c) {
    // transitions if c is acceptable, else errors
    // http://ascii-table.com/


    bool run = true; // run = false on quit command

    String disp_msg;
    sinit(&disp_msg);


    // only accept ascii characters and newline
    if(0x20 <= c && c <= 0x7e) { // if printable
        if(0x41 <= c && c <= 0x5a) {
            c |= 0x20;
        } // lower case any capital character

        switch(p->state) {
        case Empty: {
            switch(c) {
                case 't': p->state = TR_T;
                case 'r': p->state = RV_R;
                case 's': p->state = SW_S;
                case 'q': p->state = Q_Q;
                default:  p->state = Error; break;
            }
            break;
        }
        case Error: {
            break;
        }

        // ----------- tr train_number train_speed ----------- //
        case TR_T: {
            REQUIRE('t', TR_R);
            break;
        }
        case TR_R: {
            REQUIRE('r', TR_space_1);
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
            REQUIRE(' ', RV_V);
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

        // sw switch_number switch_direction ----------- //
        case SW_S: {
            REQUIRE('w', SW_W);
            break;
        }
        case SW_W: {
            REQUIRE(' ', SW_space_1);
            break;
        }
        case SW_space_1: {
            p->data.junction.switch_number = 0;
            p->state =  append_number(c, &(p->data.junction.switch_number)) ?
                        SW_switch_number :
                        Error;
            break;
        }
        case SW_switch_number: {
            if(! append_number(c, &(p->data.junction.switch_number))) {
                REQUIRE(' ', SW_space_2);
            }
            break;
        }
        case SW_space_2: {
            switch(c) { // either 'c' or 's'
            case 'c':
                p->data.junction.curved = true;
                p->state = SW_switch_dir;
                break;
            case 's':
                p->data.junction.curved = true;
                p->state = SW_switch_dir;
            default:
                p->state = Error;
            }
            break;
        }
        case SW_switch_dir: {
            p->state = Error;
            break;
        }

        // ----------- q ----------- //
        case Q_Q: {
            p->state = Error;
            break;
        }

        default:
            break;
        } // switch

        if(p->state == Error) {
            sputstr(&disp_msg, VT_COLOR_RED);
        }

        sputc(&disp_msg, c);

    } // if printable

    else if(c == VT_CARRIAGE_RETURN) { // user pressed return (enter)

    } // else if carriage return
    return run;
}


void parserTask() {
    Parser p;
    p.state = Empty;
    bool run = true; // set to false when we detect quit command
    String str;

    // TODO: draw the parsing window, etc


    // read input
    while(run) {
        sinit(&str);
        GetString(COM2, &str);
        for(unsigned i = 0; i < slen(&str); i++) {
            run = parse(&p, str.buf[i]);
        }
    }

    // write output
    Exit();
}

void initParser() {
    Create(PRIORITY_PARSER, parserTask);
}
