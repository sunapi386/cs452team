#include <user/parser.h>
#include <priority.h>
#include <utils.h>
#include <user/vt100.h>
#include <user/syscall.h> // Create, Time
#include <user/turnout.h> // to handle sw
#include <user/train.h> // to handle sw
#include <kernel/task.h> // to call taskDisplayAll()
#include <user/engineer.h> // createEngineer
#include <user/trackserver.h> // since the typedef for SensorData is moved there..
#include <debug.h>

// Parser is a giant state machine
// tr train_number train_speed
// rv train_number
// sw turnout_number turnout_direction
// q
#include <user/sensor.h> // SensorHalt

static char *help_message =
"------------------ Trains commands\r\n"
"tr train_num speed                 | set TRain speed\r\n"
"rv train_num                       | ReVerse train\r\n"
"sw train_num direction             | SWitch a turnout\r\n"
"e train_num sensorIndex            | create Engineer for train and given initial sensor node\r\n"
"h train_num group_char sensor_num  | Halts on sensor \r\n"
"0                                  | set last train speed to 0\r\n"
"c tr_num speed char num loops      | Calibration: at speed delay loop\r\n"
"k track_A_or_B_char                | load tracK A or B\r\n"
"x track_node_index distance        | x mark the track node\r\n"
"go train_num track_node_index      | \r\n"
"------------------ Misc. commands\r\n"
"q                                  | Quit\r\n"
"o                                  | redraw turnOuts\r\n"
"p                                  | Print track\r\n"
"d                                  | Debug tasks\r\n"
"?                                  | help?\r\n";

typedef struct {
    int tid;
    int trainNumber;
} EngineerInfo;

static int numEngineer = 0;
static EngineerInfo engineers[MAX_NUM_ENGINEER];

void initEngineerInfo()
{
    numEngineer = 0;
    for (int i = 0; i < MAX_NUM_ENGINEER; i++)
    {
        engineers[i].tid = 0;
        engineers[i].trainNumber = 0;
    }
}

int indexFromTrainNumber(int trainNumber)
{
    // linear search for train
    for (int i = 0; i < numEngineer; i++)
    {
        if (trainNumber == engineers[i].trainNumber)
        {
            return i;
        }
    }
    return -1;
}

int engineerCreate(int trainNumber, int sensorIndex)
{
    uassert(trainNumber >= 1 && trainNumber <= 80);
    uassert(sensorIndex >= 0 && sensorIndex < 80);

    // check if we reached max number of engineers allowed
    if (numEngineer == MAX_NUM_ENGINEER)
    {
        printf(COM2, "Unable to create engineer; max number reached: %d\n\r", MAX_NUM_ENGINEER);
        return -1;
    }

    // check to see if train is already assigned
    if (indexFromTrainNumber(trainNumber) >= 0)
    {
        printf(COM2, "Unable to create engineer; train %d is already assigned\n\r", trainNumber);
        return -1;
    }

    // create the engineer task
    int tid = Spawn(PRIORITY_ENGINEER, engineerServer, (void *)numEngineer);

    // initialize the engineer with trainNumber and initialSensorNode
    EngineerMessage initMessage;
    initMessage.type = initialize;
    initMessage.data.initialize.trainNumber = trainNumber;
    initMessage.data.initialize.sensorIndex = sensorIndex;
    int len = Send(tid, &initMessage, sizeof(initMessage), 0, 0);
    uassert(len == 0);

    // record newly created Tid
    engineers[numEngineer].tid = tid;
    engineers[numEngineer].trainNumber = trainNumber;
    numEngineer++;
    printf(COM2, "Engineer %d -> train %d\n\r", tid, trainNumber);
    return 0;
}

void engineerSetSpeed(int tid, int speed)
{
    uassert(tid >= 0);

    printf(COM2, "Engineer %d setting speed %d\n\r", tid, speed);

    EngineerMessage message;
    message.type = setSpeed;
    message.data.setSpeed.speed = speed;
    int ret = Send(tid, &message, sizeof(EngineerMessage), 0, 0);
    uassert(ret >= 0);
}

void engineerSetReverse(int tid) {
    uassert(tid >= 0);

    EngineerMessage message;
    message.type = setReverse;
    int ret = Send(tid, &message, sizeof(EngineerMessage), 0, 0);
    uassert(ret >= 0);
}

void engineerXMarksTheSpot(int tid, int index, int offset) {
    uassert(tid > 0);
    uassert(0 <= index && index <= 143);

    EngineerMessage message;
    message.type = xMark;
    message.data.xMark.index = index;
    message.data.xMark.offset = offset * 1000; // convert mm to um
    Send(tid, &message, sizeof(EngineerMessage), 0, 0);
}

void engineerGo(int tid, int index)
{
    uassert(tid > 0);
    uassert(0 <= index && index <= 143);

    EngineerMessage message;
    message.type = go;
    message.data.go.index = index;
    Send(tid, &message, sizeof(message), 0, 0);
}

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
        E,          // e train_number train_speed an engineer to drive at speed
        E_space_1,
        E_train_number,
        E_space_2,
        E_sensor_index,
        HELP,       // ? for printing all available commands
        C,          // c tr_num speed grp_char snsr_num (calibration)
        C_space_1,
        C_train_number,
        C_space_2,
        C_train_speed,
        C_space_3,
        C_sensor_group,
        C_space_4,
        C_sensor_number,
        C_space_5,
        C_loops,
        Zero,       // 0 set last train speed 0
        K,          // k A or k B (load track data)
        K_space,
        K_which_track,
        X,          // x 50, for example
        X_space1,
        X_node_number,
        X_space2,
        X_distance,
        G,          // go train_num node_num
        GO,
        GO_,
        GO_t,
        GO_t_,
        GO_t_n,
    } state;

    // store input data (train number and speed) until ready to use
    union Data {
        struct { // TR commands
            int train_number;
            int train_speed;
        } speed;
        struct { // SW commands
            int turnout_number;
            char direction;
        } turnout;
        struct { // RV data
            int train_number;
        } reverse;
        struct {
            int train_number;
            char sensor_group;
            int sensor_number;
        } sensor_halt;
        struct {
            int trainNumber;
            int sensorIndex;
        } engineer;
        struct {
            int train_number;
            int train_speed;
            char sensor_group;
            int sensor_number;
            int num_loops;
        } calibration;
        struct {
            char which_track;
        } track;
        struct {
            int node_number;
            int distance;
        } x_marks_the_spot;
        struct Go {
            int train_number;
            int node_number;
        } go;
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
                    case 'g': p->state = G; break;
                    case 't': p->state = TR_T; break;
                    case 'r': p->state = RV_R; break;
                    case 's': p->state = SW_S; break;
                    case 'q': p->state = Q_Q; break;
                    case 'd': p->state = DB_TASK; break;
                    case 'h': p->state = H_H; break;
                    case 'p': p->state = P; break;
                    case 'o': p->state = O; break;
                    case 'e': p->state = E; break;
                    case 'c': p->state = C; break;
                    case 'k': p->state = K; break;
                    case 'x': p->state = X; break;
                    case '?': p->state = HELP; break;
                    default:  p->state = Error; break;
                }
                break;
            }
            case G: {
                REQUIRE('O', GO);
                break;
            }
            case GO: {
                REQUIRE(' ', GO_);
                break;
            }
            case GO_: {
                p->data.go.train_number = 0;
                p->state =  append_number(c, &(p->data.go.train_number)) ?
                            GO_t :
                            Error;
                break;
            }
            case GO_t: {
                if(! append_number(c, &(p->data.go.train_number))) {
                    REQUIRE(' ', GO_t_);
                }
                break;
            }
            case GO_t_: {
                p->data.go.node_number = 0;
                p->state =  append_number(c, &(p->data.go.node_number)) ?
                            GO_t_n :
                            Error;
                break;
            }
            case GO_t_n: {
                if(! append_number(c, &(p->data.go.node_number))) {
                    p->state = Error;
                }
                break;
            }
            case X: {
                REQUIRE(' ', X_space1);
                break;
            }          // x A 10 (tries to stop on sensor A10 for example)
            case X_space1: {
                p->data.x_marks_the_spot.node_number = 0;
                p->state =  append_number(c, &(p->data.x_marks_the_spot.node_number)) ?
                            X_node_number :
                            Error;
                break;
            }
            case X_node_number: {
                if(! append_number(c, &(p->data.x_marks_the_spot.node_number))) {
                    REQUIRE(' ', X_space2);
                }
                break;
            }
            case X_space2: {
                p->data.x_marks_the_spot.distance = 0;
                p->state =  append_number(c, &(p->data.x_marks_the_spot.distance)) ?
                            X_distance :
                            Error;
                break;
            }
            case X_distance: {
                if(! append_number(c, &(p->data.x_marks_the_spot.distance))) {
                    p->state = Error;
                }
                break;
            }

            // ----------- k A or B (load track data) --- //
            case K: {
                REQUIRE(' ', K_space);
                break;
            }
            case K_space: {
                if('a' <= c && c <= 'b') {
                    p->data.track.which_track = c;
                    p->state = K_which_track;
                }
                else {
                    p->state = Error;
                }
                break;
            }
            case K_which_track: {
                p->state = Error;
                break;
            }
            // ----------- c tr_num speed grp_char snsr_num (calibration) ----//
            case C: {
                REQUIRE(' ', C_space_1);
                break;
            }
            case C_space_1: {
                p->data.calibration.train_number = 0;
                p->state =  append_number(c, &(p->data.calibration.train_number)) ?
                            C_train_number :
                            Error;
                break;
            }
            case C_train_number: {
                if(! append_number(c, &(p->data.calibration.train_number))) {
                    REQUIRE(' ', C_space_2);
                }
                break;
            }
            case C_space_2: {
                p->data.calibration.train_speed = 0;
                p->state =  append_number(c, &(p->data.calibration.train_speed)) ?
                            C_train_speed :
                            Error;
                break;
            }
            case C_train_speed: {
                if(! append_number(c, &(p->data.calibration.train_speed))) {
                    REQUIRE(' ', C_space_3);
                }
                break;
            }
            case C_space_3: {
                if('a' <= c && c <= 'e') {
                    p->data.calibration.sensor_group = c;
                    p->state = C_sensor_group;
                }
                else {
                    p->state = Error;
                }
                break;
            }
            case C_sensor_group: {
                REQUIRE(' ', C_space_4);
                break;
            }
            case C_space_4: {
                p->data.calibration.sensor_number = 0;
                p->state =  append_number(c, &(p->data.calibration.sensor_number)) ?
                            C_sensor_number :
                            Error;
                break;
            }
            case C_sensor_number: {
                if(! append_number(c, &(p->data.calibration.sensor_number))) {
                    REQUIRE(' ', C_space_5);
                }
                break;
            }
            case C_space_5: {
                p->data.calibration.num_loops = 0;
                p->state =  append_number(c, &(p->data.calibration.num_loops)) ?
                            C_loops :
                            Error;
                break;
            }
            case C_loops: {
                if(! append_number(c, &(p->data.calibration.num_loops))) {
                    p->state = Error;
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
                p->data.engineer.trainNumber = 0;
                p->state =  append_number(c, &(p->data.engineer.trainNumber)) ?
                            E_train_number :
                            Error;
                break;
            }
            case E_train_number: {
                if(! append_number(c, &(p->data.engineer.trainNumber))) {
                    REQUIRE(' ', E_space_2);
                }
                break;
            }
            case E_space_2: {
                p->data.engineer.sensorIndex = 0;
                p->state =  append_number(c, &(p->data.engineer.sensorIndex)) ?
                            E_sensor_index :
                            Error;
                break;
            }
            case E_sensor_index: {
                if(! append_number(c, &(p->data.engineer.sensorIndex))) {
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
////////////////////////////////////////////////////
////////////////////////////////////////////////////
////////////////////////////////////////////////////
////////////////////////////////////////////////////
    else if(c == VT_CARRIAGE_RETURN) { // user pressed return (enter)
        sputstr(&disp_msg, VT_RESET);
        sputstr(&disp_msg, "  | ");
        // should be on an end state
        switch(p->state) {
            case GO_t_n: {
                int node_number = p->data.go.node_number;
                int train_number = p->data.go.train_number;
                if (1 <= train_number && train_number <= 80 &&
                    0 <= node_number && node_number <= 143 ) {
                    engineerGo(train_number, node_number);
                }
                else {
                    sputstr(&disp_msg,
                        "GO: bad train number or node number\r\n");
                }
                break;
            }
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
                else {
                    sensorHalt(train_number, sensor_group, sensor_number);
                }
                break;
            }
            case TR_speed: {
                // check train_number and train_speed
                int train_number = p->data.speed.train_number;
                int train_speed = p->data.speed.train_speed;

                if((1 <= train_number && train_number <= 80) &&
                   (0 <= train_speed  && train_speed  <= 14))
                {
                    // get engineer tid from train number
                    int index = indexFromTrainNumber(train_number);

                    // if tid exists, send to engineer
                    if (index >= 0)
                    {
                        engineerSetSpeed(engineers[index].tid, train_speed);
                    }
                    // else, set speed directly
                    else
                    {
                        trainSetSpeed(train_number, train_speed);
                    }
                }
                else
                {
                    sputstr(&disp_msg, "TR: bad train_number or train_speed\r\n");
                }

                break;
            }
            case RV_train_number: {
                // check train_number
                int train_number = p->data.reverse.train_number;

                if(1 <= train_number && train_number <= 80)
                {
                    // get engineer tid from train number
                    int index = indexFromTrainNumber(train_number);
                    assert(index < MAX_NUM_ENGINEER);

                    // if tid exists, send to engineer
                    if (index >= 0)
                    {
                        engineerSetReverse(engineers[index].tid);
                    }
                    // else, set speed directly
                    else
                    {
                        trainSetReverse(train_number);
                    }
                }
                else
                {
                    sputstr(&disp_msg, "RV: bad train_number\r\n");
                }
                break;
            }
            case SW_turnout_dir: {
                // check turnout_number
                int turnout_number = p->data.turnout.turnout_number;
                if(   (1 <= turnout_number && turnout_number <= 18) ||
                    (153 <= turnout_number && turnout_number <= 156)) {
                    char direction = p->data.turnout.direction;
                    assert(direction == 's' || direction == 'c');
                    sputstr(&disp_msg, "turnout changing\r\n");
                    setTurnout(turnout_number, direction);
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
                sputstr(&disp_msg, "Drawing track A!\r\n");
                drawTrackLayoutGraph('a');
                break;
            }
            case C: {
                sputstr(&disp_msg, "Running calibration! Warning: parser stops working.\r\n");
                for(int i = 0; i < 5; i++) {
                    printf(COM2, "calibration loop: %d\r\n", i);
                    Delay(500); // 500 ticks * 10 ms per tick = 5 seconds

                }
                break;
            }
            case O:
            {
                sputstr(&disp_msg, "Drawing turnouts!\r\n");
                printResetTurnouts();
                break;
            }
            case E_sensor_index:
            {
                int trainNumber = p->data.engineer.trainNumber;
                int sensorIndex = p->data.engineer.sensorIndex;
                if (trainNumber < 1 || trainNumber > 80)
                {
                    sputstr(&disp_msg, "Error bad trainNumber ");
                    sputint(&disp_msg, trainNumber, 10);
                    sputstr(&disp_msg, "\r\n");
                }
                else if (sensorIndex < 0 || sensorIndex >= 80)
                {
                    sputstr(&disp_msg, "Error bad sensorIndex ");
                    sputint(&disp_msg, sensorIndex, 10);
                    sputstr(&disp_msg, "\r\n");
                }
                else
                {
                    sputstr(&disp_msg, "Create engineer for ");
                    sputint(&disp_msg, trainNumber, 10);
                    sputstr(&disp_msg, " with initial sensor ");
                    sputint(&disp_msg, sensorIndex, 10);
                    sputstr(&disp_msg, "\r\n");
                    engineerCreate(trainNumber, sensorIndex);
                }
                break;
            }
            case HELP: {
                sputstr(&disp_msg, help_message);
                break;
            }
            case X_distance: {
                int node_number = p->data.x_marks_the_spot.node_number;
                int distance = p->data.x_marks_the_spot.distance;

                if (0 <= node_number && node_number <= 139 &&
                    0 <= distance && distance <= 875)
                {
                    // if no engineers exist, error
                    if (numEngineer == 0) {
                        sputstr(&disp_msg, "Error: engineer server not created");
                        break;
                    }

                    int tid = engineers[0].tid;
                    uassert(tid > 0);

                    engineerXMarksTheSpot(tid, node_number, distance);
                }
                else {
                    sputstr(&disp_msg, "Error: XMarksTheSpot expects node number ");
                    sputint(&disp_msg, node_number, 10);
                    sputstr(&disp_msg, " between 0-139 and distance ");
                    sputint(&disp_msg, distance, 10);
                    sputstr(&disp_msg, " between 0 to 875\r\n");
                }
                break;
            }

            case K_which_track: {
                char which_track = p->data.track.which_track;
                sputstr(&disp_msg, "Loading track\n\r");
                loadTrackStructure(which_track);
                break;
            }
            case C_loops: {
                int train_number = p->data.calibration.train_number;
                int train_speed = p->data.calibration.train_speed;
                char sensor_group = p->data.calibration.sensor_group;
                int sensor_number = p->data.calibration.sensor_number;
                int num_loops = p->data.calibration.num_loops;

                if( (1 <= train_number && train_number <= 80) &&
                    (0 <= train_speed && train_speed <= 14)   &&
                    ('a' <= sensor_group && sensor_group <= 'e') &&
                    (1 <= sensor_number && sensor_number <= 16)  &&
                    (1 <= num_loops && num_loops <= 20)) {

                    sensorHalt(train_number, sensor_group, sensor_number);
                    for(int i = 0; i < num_loops; i++) {
                        trainSetSpeed(train_number, train_speed);
                        Delay(2000); // estimate 20 seconds for a run
                    }
                }
                else {
                    sputstr(&disp_msg, "Bad calibration command: ");
                    sputint(&disp_msg, train_number, 10);
                    sputstr(&disp_msg, " ");
                    sputint(&disp_msg, train_speed, 10);
                    sputstr(&disp_msg, " ");
                    sputc(&disp_msg, sensor_group);
                    sputstr(&disp_msg, " ");
                    sputint(&disp_msg, sensor_number, 10);
                    sputstr(&disp_msg, " ");
                    sputint(&disp_msg, num_loops, 10);
                    sputstr(&disp_msg, "\r\n");
                }
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
    vt_pos(&s, 9, 45);
    sputstr(&s, "Please load track.");
    vt_pos(&s, VT_PARSER_ROW, VT_PARSER_COL);
    sputstr(&s, "> ");
    PutString(COM2, &s);

    // read input
    while(run) {
        String input;
        sinit(&input);
        while(1) { // read string
            char ch = Getc(COM2);
            if(ch == VT_BACKSPACE) {
                spop(&input);
                continue;
            }
            sputc(&input, ch);
            if(ch == VT_CARRIAGE_RETURN) {
                break;
            }
        }
        // parse string
        for(unsigned i = 0; i < input.len && run; i++) {
            run = parse(&p, input.buf[i]);
        }
    }

    // write output
    Halt();
}

void initParser()
{
    initEngineerInfo();

    Create(PRIORITY_PARSER, parserTask);
}

