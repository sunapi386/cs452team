#include <user/engineer.h>
#include <priority.h>
#include <debug.h> // assert
#include <user/syscall.h>
#include <utils.h> // printf
#include <user/train.h> // trainSetSpeed

#define ENSTRUCTION_BUFFER_SIZE 5

// static void controllerSimulatorTask() {
//     int ret = RegisterAs("controller_simulator");
//     assert(ret == 0);
//     while(1) {
//         ret = Receive();
//         assert(ret ==  sizeof(Enstruction));
//     }
// }

// Clockserver interrupt frequency: 10 ms per tick.
// In train.c, reverseNicely() uses speeds[train_number] * 200 to calculate how
// long to wait for a transition.
// i.e. speed 14 means: 14 * 200 ticks = 2800 ticks delay to stop (280 ms),
// TODO: is 280ms delay correct? why does it seem like we delay for 3 seconds
// Assuming linear accleration and deceleration out:
//       2800 ticks / 14 transition intervals = 200 ticks per transition (20ms)
// Therefore, we fill accel and decel with 200.
// TODO: non-linear accel/decel means the estimates need calibration.

// Estimated velocity of a train in um/tick.
static int velocity[15] = {0,100,200,400,600,800,1000,1200,1600,2000,2400,3000,3600,4200,5000};
// Estimated transition time from one speed to the one above, in ticks.
static int accel[14] = {200,200,200,200,200,200,200,200,200,200,200,200,200,200};
// Estimated transition time into a speed from the one above, in ticks.
static int decel[14] = {200,200,200,200,200,200,200,200,200,200,200,200,200,200};
// Pre-computed transition table
static int speed_change[15][15];

static int active_train;
static void precompute() {
    for(int to = 0; to < 15; to++) {
        speed_change[to][to] = 0;
        for(int from = to - 1; from >= 0; from--) {
            speed_change[from][to] = speed_change[from + 1][to] + accel[from];
        }
        for(int from = to + 1; from < 15; from++) {
            speed_change[from][to] = speed_change[from - 1][to] + decel[from - 1];
        }
    }
}

static void engineerTask() {
    // int controller_id = WhoIs("controller_simulator");
    // assert(controller_id >= 0);
    // TODO: receive the Enstruction from controller, for now just init here

    Enstruction manuvers[ENSTRUCTION_BUFFER_SIZE];
    // for(int i = 0; i < ENSTRUCTION_BUFFER_SIZE; i++) {
        // manuvers[i] = {.speed = 0, .time = 0 , .distance = 0};
    // }
    int current_enstruction = 0;
    int current_distance = 0;
    int current_speed = 0;
    int current_time = Time(); // TODO: what about being blocked by receive?

    for(;;) {
        // an order is sent here by the human
        Enstruction ordered;
        int tid;
        int len = Receive(&tid, &ordered, sizeof(Enstruction));
        assert(len == sizeof(Enstruction));
        Reply(tid, 0, 0);

        printf(COM2, "engineer received an order: speed %d time %d distance %d",
            ordered.speed, ordered.time, ordered.distance);

        assert(0 <= current_enstruction && current_enstruction < ENSTRUCTION_BUFFER_SIZE);
        manuvers[current_enstruction].speed = ordered.speed,
        manuvers[current_enstruction].time = ordered.time,
        manuvers[current_enstruction].distance = ordered.distance,
        current_enstruction++;

        printf(COM2, "engineer now has %d enstructions", current_enstruction);

        // calc time required for changing to ordered speed
        int delta_time = speed_change[current_speed][ordered.speed];
        // calc distance required to fufil order
        int delta_distance = delta_time * (velocity[current_speed] +
                            velocity[ordered.speed]) / 2;

        int bound_time = (ordered.time - current_time);
        // should be possible to change to that speed before the ordered time
        assert(delta_time <= bound_time);
        // and that our distance to travel is no more than what is ordered
        assert(delta_distance <= (ordered.distance - current_distance));

        printf(COM2, "engineer estimates time %d and distance %d to fill order",
            delta_time, delta_distance);

        int distance_to_target = (current_distance - ordered.distance);
        int travelled_before = velocity[current_speed] * current_time;
        int travelled_after = velocity[ordered.speed] * ordered.time;
        int travelled_delta = velocity[ordered.speed] * delta_time;

        int expected_end_time =    (
                            distance_to_target -
                            travelled_before +
                            delta_distance +
                            travelled_after -
                            travelled_delta
                            )
                        /  (velocity[ordered.speed] - velocity[current_speed]);

        assert(current_time <= expected_end_time && expected_end_time <= ordered.time);

        printf(COM2, "engineer expects to fufil order at %d", expected_end_time);

        DelayUntil(expected_end_time);

        // shuffle the enstructions buffer forward
        for(int i = 0; i < current_enstruction - 1; i++) {
            manuvers[i].speed = manuvers[i + 1].speed;
            manuvers[i].time = manuvers[i + 1].time;
            manuvers[i].distance = manuvers[i + 1].distance;
        }
        current_enstruction--;

        // when woken up again, change the state of the variables
        current_time = ordered.time;
        current_speed = ordered.speed;
        current_distance = ordered.distance;

        // set train speed
        trainSetSpeed(active_train, current_speed);
    }

}

static int engineerTaskId;
void initEngineer() {
    precompute();

    // Create(PRIORITY_TRACK_CONTROLLER_TASK, controllerSimulatorTask);
    engineerTaskId = Create(PRIORITY_ENGINEER, engineerTask);
    assert(engineerTaskId >= 0);
}

int engineerPleaseManThisTrain(int train_number) {
    assert(engineerTaskId >= 0);
    active_train = train_number;
    return engineerTaskId;
}
