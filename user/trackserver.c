#include <debug.h>
#include <priority.h>
#include <user/vt100.h>
#include <user/syscall.h>
#include <user/track_data.h>
#include <user/nameserver.h>
#include <user/trackserver.h>

track_node g_track[TRACK_MAX];

void trackServer()
{
    int tid;
    TrackRequest req;

    RegisterAs("trackServer");

    for (;;)
    {
        // Receive the message
        Receive(&tid, &req, sizeof(req));

        // debug("train controller Received from tid %d", tid);
        // Reply(tid, 0, 0);

        // switch on the type of the message
        switch (req.type)
        {
        case type_sensor:
            break;
        case type_turnout:
            break;
        default:
            break;
        }
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
