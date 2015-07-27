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
E.g. (path1) C12 -> MR14 <- MR11 -> C13 ->. The reversing at MR14 actually
requires reservation of A3/A4 sensor track_node because the distance from
MR14 to A3 is just 6 cm (shorter than a train's length).
So path1 is expanded to (path2):
[C12,43] ->  [MR14,107] ->  [A4,3] <-  [MR11,101] ->  [C13,44] ->.
*/
int expandPath(PathBuffer *pb);

/**
Takes a path buffer and converts it into a Ebook, which is a series
of engineer instructions (enstructions).
Returns the number of enstructions, 0 to MAX_EBOOK_LENGTH.
Or -1 on error.
E.g. path2 [C12,43] ->  [MR14,107] ->  [A4,3] <-  [MR11,101] ->  [C13,44] ->
creates the following enstructions:
1/ Togo MR14. Switch false.
2/ Togo C13. Switch false.
*/
int makeEbook(PathBuffer *pb, Ebook *book);
void printEnstruction(Enstruction *en);
void printEbook(Ebook *book);
void printPath(PathBuffer *pb);
int distanceBetween(track_node *from, track_node *to);
track_edge *getNextEdge(track_node *node);
track_node *getNextNode(track_node *currentNode);
track_node *getNextSensor(track_node *node);

#endif
