#ifndef __PATHFINDING_H
#define __PATHFINDING_H
#include <user/track_data.h>
#include <utils.h> // bool

/**
Position for XMarksTheSpot stopping.
*/
typedef struct position {
    track_node *node;
    int offset;
} Position;

#define MAX_PATH_LENGTH (TRACK_MAX)// very generous
/**
Turnout operation. The idea is that the engineer follows a series of these.
*/
typedef int Turnop; // (switch_number << 1) | (1 bit curved)

/**
Enstruction to pass to the engineer, in a sequential manner of exactly
where he should goto and what track nodes he should reserve.
*/
typedef struct {
    int id;
    int length;
    Position togo;      /* because goto is a reserved keyword */
    Turnop turnops[MAX_PATH_LENGTH];
    track_node *tracknodes[MAX_PATH_LENGTH];
} Enstruction;

/**
Ebook is a list of enstructions. Size
*/
#define MAX_EBOOK_LENGTH  (MAX_PATH_LENGTH / 10)
typedef struct ebook {
    Enstruction enstructs[MAX_EBOOK_LENGTH];
    int length;
} Ebook;

/**
PathBuffer for passing around paths.
Array of track_node indecies, ordered from destination (0) to source (length).
*/
typedef struct PathBuffer {
    int train_num;   /* Engineer: train_number for reservation planning */
    int length;
    track_node *tracknodes[MAX_PATH_LENGTH];
    bool reverse[MAX_PATH_LENGTH];
} PathBuffer;

/**
planRoute
Returns -1 if no path exists, otherwise the length of the path.
*/
int planRoute(track_node *start, track_node *end, PathBuffer *pb);

/**
Returns an expanded version of the path, so the engineer can make reservations.
Some reversing paths are too short and need more reservations.
E.g. C12 -> MR14 <- MR11 -> C13. In this case reversing at MR14 actually
requires reservation of A3/A4 sensor track_node because the distance from
MR14 to A3 is just 6 cm (shorter than a train's length).
*/
int expandPath(PathBuffer *pb);

void printPath(PathBuffer *pb);
int distanceBetween(track_node *from, track_node *to);
track_edge *getNextEdge(track_node *node);
track_node *getNextNode(track_node *currentNode);
track_node *getNextSensor(track_node *node);

#endif
