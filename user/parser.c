#include <user/parser.h>
#include <priority.h>
#include <utils.h>
#include <string.h>
#include <user/io.h>
#include <user/vt100.h>
#include <user/syscall.h> // Create
#include <user/train.h>

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

        // should be on an end state
        switch(p->state) {
            case TR_speed: {
                // check train_number and train_speed
                int train_number = p->data.speed.train_number;
                int train_speed = p->data.speed.train_speed;
                if((0 <= train_number && train_number <= 80) &&
                   (0 <= train_speed  && train_speed  <= 14)) {
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
                    trainSetReverse(train_number);
                }
                else {
                    sputstr(&disp_msg,
                        "RV: bad train_number\r\n");
                }

                break;
            }
            case SW_switch_dir: {
                // check switch_number
                int switch_number = p->data.junction.switch_number;
                if(   (1 <= switch_number && switch_number <= 18) ||
                    (153 <= switch_number && switch_number <= 156)) {
                    bool curved = p->data.junction.curved;
                    trainSetSwitch(switch_number, curved);
                }
                else {
                    sputstr(&disp_msg,
                        "SW: bad switch_number\r\n");
                }

                break;
            }
            case Q_Q: {
                sputstr(&disp_msg, "Quit\r\n");
                run = false;
                break;
            }
            default: {
                sputstr(&disp_msg, "Command not recognized!\r\n");
            }
        } // switch
        p->state = Empty;
    } // else if carriage return
    PutString(&disp_msg);
    return run;
}


void parserTask() {
    Parser p;
    p.state = Empty;
    bool run = true; // set to false when we detect quit command

    // draw the parsing window, etc
    String s;
    sinit(&s);
    // vtMove(&s, VT_PARSER_ROW, VT_PARSER_COL);
    sputstr(&s, VT_CSI);
    sputuint(&s, VT_PARSER_ROW, 10);
    // sputstr(&s,)
    sputuint(&s, VT_PARSER_COL, 10);
    sputstr(&s, "> ");
    PutString(&s);

    // read input
    while(run) {
        char ch = Getc(COM2);
        run = parse(&p, ch);
    }

    // write output
    Exit();
}

void initParser() {
    Create(PRIORITY_PARSER, parserTask);
}
