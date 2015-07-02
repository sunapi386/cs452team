#include <user/engineer.h>
#include <priority.h>
#include <debug.h> // assert
#include <user/syscall.h>

#define NUM_ENSTRUCTIONS 5

// static void controllerSimulatorTask() {
//     int ret = RegisterAs("controller_simulator");
//     assert(ret == 0);
//     while(1) {
//         ret = Receive();
//         assert(ret ==  sizeof(Enstruction));
//     }
// }


static void engineerTask() {

    // int controller_id = WhoIs("controller_simulator");
    // assert(controller_id >= 0);
    // TODO: receive the Enstruction from controller, for now just init here
    // Enstruction instructions[NUM_ENSTRUCTIONS];
    // for(int i = 0; i < NUM_ENSTRUCTIONS; i++) {
        // instructions[i] = {0};
    // }


    for(;;) {
        Enstruction enstruction;
        int tid;
        int len = Receive(&tid, &enstruction, sizeof(Enstruction));
        assert(len == sizeof(Enstruction));


    }

}

void initEngineer() {
    // Create(PRIORITY_TRACK_CONTROLLER_TASK, controllerSimulatorTask);
    Create(PRIORITY_ENGINEER, engineerTask);
}
