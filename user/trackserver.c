#include <user/trackserver.h>
#include <debug.h>
#include <priority.h>
#include <user/syscall.h>
#include <user/nameserver.h>
#include <user/pathfinding.h> // getNextEdge

static bool is_reservable(struct Reservation *r) {
    int num_requested = r->num_requested;
    for (int i = 0; i < num_requested; i++) {
        // ensure they're not already reserved, even by itself
        if (r->nodes[i]->owner != -1 ||
            r->nodes[i]->reverse->owner != -1) {
            return false;
        }
        // should not give out reservations too close to branch or merge
        if (r->nodes[i]->type == NODE_MERGE ||
            r->nodes[i]->type == NODE_BRANCH) {
            if (getNextNode(r->nodes[i])->owner != -1 ||
                getNextNode(r->nodes[i]->reverse)->owner != -1) {
                return false;
            }
        }
    }
    // engineer should ensure track requests are continguous
    return true;
}

static bool is_owner(struct Reservation *r) {
    for (int i = 0; i < r->num_requested; i++) {
        if (r->nodes[i]->owner != r->train_num) {
            return false;
        }
    }
    return true;
}

/**
Reservation system:
A track_node an edge, which contains the track distance and represents the track
to be reserved. On a reservation request, the edge and its reverse direction
is checked that it is free.
*/
void trackServer() {
    RegisterAs("trackServer");
    TrackServerMessage req;
    int tid;
    for (;;) {
        Receive(&tid, &req, sizeof(req));
        switch (req.type) {
            case RESERVATION: {
                int num_requested = req.reservation.num_requested;
                int success;
                if (req.reservation.op == RESERVE) {
                    if (! is_reservable(&req.reservation)) {
                        success = 0;
                        Reply(tid, &success, sizeof(int));
                        break;
                    }
                    // mark them as reserved
                    int train_num = req.reservation.train_num;
                    for(int i = 0; i < num_requested; i++) {
                        req.reservation.nodes[i]->owner = train_num;
                        req.reservation.nodes[i]->reverse->owner = train_num;
                    }
                    success = 1;
                    Reply(tid, &success, sizeof(int));
                }
                else if (req.reservation.op == RELEASE) {
                    /**
                    Check engineer releasing was actually given that track.
                    What does it mean to fail to release tracks?
                    */
                    uassert(is_owner(&req.reservation));
                    for(int i = 0; i < req.reservation.num_requested; i++) {
                        req.reservation.nodes[i]->owner = -1;
                        req.reservation.nodes[i]->reverse->owner = -1;
                    }
                }
                break;
            }
            case QUIT:
                Reply(tid, 0, 0);
                goto cleanup;

            default:
                break;
        }
    }
cleanup:
    Exit();
}

static int trackserverid;
void initTrackServer() {
    trackserverid = Create(PRIORITY_TRACKSERVER, trackServer);
}

void exitTrackServer() {
    TrackServerMessage msg = {.type = QUIT};
    Send(trackserverid, &msg, sizeof(msg), 0, 0);
}
