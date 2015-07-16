#include <user/pathfinding.h>

#define MAX_NODES_PATH 80

typedef struct PathNode {
    track_node *track_node;
    struct PathNode *from;
    int cost;
    int total;
} PathNode;




void planRoute(struct track_node *src, struct track_node *dst) {
    assert(src);
    assert(dst);
    struct track_node *route[MAX_NODES_PATH];
    int cost;
    int count = find_path(src, dst, route, &cost);
    print_route(route);

}

static void printRoute(track_node **route) {
    String s;
    for(int i = 0; i < count; ++i){
        sinit(&s);
        sputstr(&s, "-> ");
        sputstr(&s,route[i]->name);
    }

    sinit(&s);
    sputstr(&s, "Cost: ");
    sputint(&s, cost, 10);


}

int distanceBetween(track_node *from, track_node *to) {
    assert(from != 0);
    assert(to != 0);

    int total_distance = 0;
    int i;
    track_node *next_node = from;
    for(i = 0; i < TRACK_MAX; i++) {
        // printf(COM2, "[%s].", next_node->name);
        if(next_node->type == NODE_BRANCH) {
            // char direction = (turnoutIsCurved(next_node->num) ? 'c' : 's');
            // printf(COM2, "%d(%c,v%d)\t",
            //      next_node->num, direction, turnoutIsCurved(next_node->num));
            total_distance += next_node->edge[turnoutIsCurved(next_node->num)].dist;
            next_node = next_node->edge[turnoutIsCurved(next_node->num)].dest;
        }
        else if(next_node->type == NODE_SENSOR ||
                next_node->type == NODE_MERGE) {
            // printf(COM2, ".%s\t", next_node->edge[DIR_AHEAD].dest->name);

            total_distance += next_node->edge[DIR_AHEAD].dist;
            next_node = next_node->edge[DIR_AHEAD].dest;
        }
        else {
            // printf(COM2, ".?\t");
            break;
        }
        // TODO: next_node = getNextLandmark(next_node);
        if(next_node == to) {
            // printf(COM2, "\r\ndistanceBetween: total_distance %d.\n\r", total_distance);
            break;
        }
    }
    if(i == TRACK_MAX) {
        printf(COM2, "distanceBetween: something went wrong: %d.\n\r", i);
    }
    return 1000 * total_distance;
}

track_edge *getNextEdge(track_node *current_landmark) {
    if(current_landmark->type == NODE_BRANCH) {
        return &current_landmark->edge[turnoutIsCurved(current_landmark->num)];
    }
    else if(current_landmark->type == NODE_SENSOR ||
            current_landmark->type == NODE_MERGE) {
        return &current_landmark->edge[DIR_AHEAD];
    }
    else {
        printf(COM2, "gne?");
    }
    return 0;

}

// function that looks up the next landmark, using direction_is_forward
// returns a landmark
track_node *getNextLandmark(track_node *current_landmark) {
    track_edge *next_edge = getNextEdge(current_landmark);
    return (next_edge == 0 ? 0 : next_edge->dest);
}