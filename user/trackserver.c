#include <user/trackserver.h>
#include <debug.h>
#include <priority.h>
#include <user/vt100.h>
#include <user/syscall.h>
#include <user/track_data.h>
#include <user/nameserver.h>
#include <user/pathfinding.h> // getNextEdge
#include <user/turnout.h>

track_node g_track[TRACK_MAX];

// A track node is reservable when
// 1. owner == -1 || owner == tid
// 2. node->reverse->owner == -1
static inline bool isReservable(track_node *node, int tid)
{
    uassert(node != 0);
    track_node *reverse = node->reverse;
    uassert(reverse != 0);

    return (node->owner == -1 || node->owner == tid) && (reverse->owner == -1);
}

/**
    Reservation system:
        A track_node an edge, which contains the track distance and represents the track
        to be reserved. On a reservation request, the edge and its reverse direction
        is checked that it is free.
*/

void trackServer() {
    RegisterAs("trackServer");
    int tid = 0;
    TrackServerReply reply;
    TrackServerMessage message;
    initTurnouts();

    for (;;) {
        int len = Receive(&tid, &message, sizeof(message));
        uassert(len == sizeof(TrackServerMessage));

        int numReserve = message.numReserve;
        int numRelease = message.numRelease;
        uassert(numReserve > 0 && numRelease > 0);

        reply = Success;

        // renounce ownership of each node in this array
        // if the caller does not own this track, then warn and do nothing
        // else return ReleaseSuccess
        for (int i = 0; i < numRelease; i++)
        {
            track_node *releaseNode = message.releaseNodes[i];
            uassert(releaseNode != 0);
            if (releaseNode->owner != -1 && releaseNode->owner != tid)
            {
                printf(COM2, "Tid %d trying to release node %s that doesn't belong to it", tid, releaseNode->name);
            }
            releaseNode->owner = -1;
        }

        // try to reserve each node in the reserve array,
        // if fails, then reply ReservationFailure
        // else, reply ReservationSuccess
        for (int i = 0; i < numReserve; i++)
        {
            track_node *reserveNode = message.reserveNodes[i];
            uassert(reserveNode != 0);

            if (!isReservable(reserveNode, tid))
            {
                if (reserveNode->owner != -1 && reserveNode->owner != tid)
                {
                    reply = ReserveFailSameDir;
                }
                else
                {
                    // head on collision
                    reply = ReserveFailOppositeDir;
                }
                break;
            }
        }

        Reply(tid, &reply, sizeof(reply));
    }
    Exit();
}

void drawTrackLayoutGraph(char track) {
    char *trackA =
        "\033[2;44H"    "-- Track A --"
        "\033[4;24H"    "---------12--11----------------------------------\\"
        "\033[5;24H"    "-------4/   /  ---------13--------10-----------\\  \\"
        "\033[6;24H"    "          14--/           \\   |  /              \\  \\"
        "\033[7;24H"    "          /                \\  |/155              \\--9"
        "\033[8;24H"    "         |                156\\|                    |"
        "\033[9;24H"    "         |                    |\\154                |"
        "\033[10;24H"   "          \\               153/|  \\               /--8"
        "\033[11;24H"   "          15--\\            /  |   \\             /  /"
        "\033[12;24H"   "--------\\   \\  ----------S---------S-----------/  /"
        "\033[13;24H"   "---------1\\  \\---------6------------7------------/"
        "\033[14;24H"   "-----------2\\           \\          /"
        "\033[15;24H"   "-------------3-----------18--------5------------";

    char *trackB =
        "\033[2;44H"    "-- Track B --"
        "\033[4;24H"    "        ------------5--------18-----------3-------------"
        "\033[5;24H"    "                    /          \\           \\2-----------"
        "\033[6;24H"    "      /------------7------------6---------\\  \\1---------"
        "\033[7;24H"    "     /  /-----------17--------16---------  \\   \\        "
        "\033[8;24H"    "    /  /              \\  |  /            \\--15  \\-\\     "
        "\033[9;24H"    "   8--/                \\ |/153               \\     -\\   "
        "\033[10;24H"   "   |                 154\\|                    |     |   "
        "\033[11;24H"   "   |                     |\\156                |     |   "
        "\033[12;24H"   "  9--\\               155/|  \\                /    /-/   "
        "\033[13;24H"   "    \\  \\              /  |   \\           /--14   /      "
        "\033[14;24H"   "     \\  \\-----------10--------13---------  /   /4-------"
        "\033[15;24H"   "      \\----------------------------------11--12---------";

    assert(STR_MAX_LEN > strlen(trackA));
    assert(STR_MAX_LEN > strlen(trackB));

    String s;
    sinit(&s);
    sputstr(&s, VT_CURSOR_SAVE);
    vt_pos(&s, VT_TRACK_GRAPH_ROW, VT_TRACK_GRAPH_COL);
    sputstr(&s, VT_RESET);
    sputstr(&s, track == 'a' ? trackA : trackB);
    sputstr(&s, VT_RESET);
    sputstr(&s, VT_CURSOR_RESTORE);
    PutString(COM2, &s);
}

void loadTrackStructure(char track) {
    // draw track
    drawTrackLayoutGraph(track);
    if(track == 'a') {
        init_tracka(g_track);
    }
    else {
        init_trackb(g_track);
    }
}

void initTrackServer() {
    Create(PRIORITY_TRACKSERVER, trackServer);
}
