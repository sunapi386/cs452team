#include <user/parser.h>
#include <priority.h>
#include <utils.h>
#include <string.h>
#include <io.h>

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
        struct Speed {
            int train_number;
            int train_speed;
        } speed;
        struct Junction { // can't call it switch, reserved keyword
            int switch_number;
            bool curved;
        } junction;
        struct Reverse {
            int train_number;
        } reverse;
    } data;

} Parser;


static bool parse(Parser *p, char c) {
    // takes parser, looks at the current state and a new char c,
    // does a transition if c is acceptable
    // otherwise give error
    bool run = true;
    // only accept ascii characters and newline
    if(0x20 <= c && c <= 0x7e) { // if printable
        if(0x41 <= c && c <= 0x5a) { c |= 0x20; } // lower case CAPS characters
        switch(p->state) {
        case Empty: {
            switch(c): {
                case 't': p->state = TR_T;
                case 'r': p->state = RV_R;
                case 's': p->state = SW_S;
                case 'q': p->state = Q_Q;
                default:  p->state = Error; break;
            } // switch
            break;
        } // Empty
        case Error: {
            break;
        } // Error
        case TR_T: {
            break;
        } // TR_T
        case TR_R: {
            break;
        } // TR_R
        case TR_space_1: {
            break;
        } // TR_space_1
        case TR_train_number: {
            break;
        } // TR_train_number
        case TR_space_2: {
            break;
        } // TR_space_2
        case TR_speed: {
            break;
        } // TR_speed
        case RV_R: {
            break;
        } // RV_R
        case RV_V: {
            break;
        } // RV_V
        case RV_space: {
            break;
        } // RV_space
        case RV_train_number: {
            break;
        } // RV_train_number
        case SW_S: {
            break;
        } // SW_S
        case SW_W: {
            break;
        } // SW_W
        case SW_space_1: {
            break;
        } // SW_space_1
        case SW_switch_number: {
            break;
        } // SW_switch_number
        case SW_space_2: {
            break;
        } // SW_space_2
        case SW_switch_dir: {
            break;
        } // SW_switch_dir
        case Q_Q: {
            break;
        } // Q_Q

        } // switch
    } // if printable
    return run;
}


void parserTask() {
    Parser parser;
    p.state = Empty;
    bool run = true; // set to false when we detect quit command
    String str;

    // TODO: draw the parsing window, etc


    // read input
    while(run) {
        sinit(&str);
        GetString(COM2, &str);
        for(unsigned i = 0; i < slen(&str); i++) {
            run = parse(&parser, ch);
        }
    }

    // write output
    Exit();
}

void initParser() {
    Create(PRIORITY_PARSER, parserTask);
}
