#include <assert.h> //assert
#include <stdio.h> //printf
#include <stdlib.h>

/**
Preprocessor generated template for heap binary heap (priority queue) structure.
*/
#define LEFT(i) (i<<1)
#define RIGHT(i) ((i<<1) + 1)
#define PARENT(i) (i>>1)

#define DECLARE_HEAP(heaptype, valuetype, value_func, height, comp)\
    struct heaptype {\
        valuetype heap[1+(1<<height)];\
        int size;\
    };\
    \
    void swap(valuetype *node1, valuetype *node2) {\
        valuetype temp = *node1;\
        *node1 = *node2;\
        *node2 = temp;\
    }\
    \
    void printheap(struct heaptype *heap) {\
        printf("printheap size %d ", heap->size);\
        for (int i = 1; i <= heap->size; i++) {\
            printf("%c ", heap->heap[i]->ch);\
        }\
        printf("\n");\
    }\
    void heaptype ## SiftUp(struct heaptype *heap, int position){\
        if(position == 1) return;\
        if(value_func(heap->heap[position]) comp value_func(heap->heap[PARENT(position)])){\
            swap(&heap->heap[position], &heap->heap[PARENT(position)]);\
            heaptype ## SiftUp(heap, PARENT(position));\
        }\
        printf("SiftUp ");\
        printheap(heap);\
    }\
    \
    void heaptype ## SiftDown(struct heaptype *heap, int position){\
        if(LEFT(position) <= heap->size && \
        value_func(heap->heap[LEFT(position)]) comp value_func(heap->heap[position])){\
            swap(&heap->heap[position], &heap->heap[LEFT(position)]);\
            heaptype ## SiftDown(heap, LEFT(position));\
        }\
        if(RIGHT(position) <= heap->size && \
        value_func(heap->heap[RIGHT(position)]) comp value_func(heap->heap[position])){\
            swap(&heap->heap[position], &heap->heap[RIGHT(position)]);\
            heaptype ## SiftDown(heap, RIGHT(position));\
        }\
        printf("SiftDown ");\
        printheap(heap);\
    }\
    \
    void heaptype ## Push(struct heaptype *heap, valuetype value){\
        assert(heap->size < (1<<height));\
        heap->heap[++heap->size] = value;\
        heaptype ## SiftUp(heap, heap->size);\
    }\
    \
    valuetype heaptype ## Pop(struct heaptype *heap){\
        assert(heap->size > 0);\
        valuetype ret = heap->heap[1];\
        heap->heap[1] = heap->heap[heap->size--];\
        heaptype ## SiftDown(heap, 1);\
        return ret;\
    }


struct CharNode {
    char ch;
    int rank;
};

inline static int rank(struct CharNode *node){
    return node->rank;
}


DECLARE_HEAP(CharHeap, struct CharNode*, rank, 5, <);

int main(int argc, char const *argv[]) {

    // struct CharNode* a = 0x0010, * b = 0x0020;
    // printf("a 0x%u, b 0x%u\n", a, b );
    // swap(&a, &b);
    // printf("a 0x%u, b 0x%u\n", a, b );

    // return 0;

    if (argc != 2) {
        printf("Usage: %s chars\n", argv[0]);
        return -1;
    }
    int CHARS = atoi(argv[1]);
    struct CharNode nodes[CHARS];
    for(int i = 0; i < CHARS; i++) {
        char c = i + 'a';
        nodes[i].ch = c;
        nodes[i].rank = i;
        // printf("%c %d\n", c, i);
    }

    struct CharHeap heap;
    heap.size = 0;
/*
{   // Push forward a-z
    for(int i = 0; i < CHARS; i++) {
        struct CharNode *node = &nodes[i];
        printf("Pushing CharNode: %d %c\n", node->rank, node->ch);
        CharHeapPush(&heap, &nodes[i]);

        for(int i = 1; i <= heap.size; i++) {
            printf("%c", heap.heap[i]->ch);
            // printf("%d ", heap.heap[i]->rank);
        }
        printf("\n");
    }
}
*/
{   // Push backwards z-a
    for(int i = CHARS-1; i >= 0; i--) {
        struct CharNode *node = &nodes[i];
        CharHeapPush(&heap, &nodes[i]);
        printf("Pushing CharNode: %d %c\n", node->rank, node->ch);

        for(int i = 1; i <= heap.size; i++) {
            printf("%c", heap.heap[i]->ch);
            // printf("%d ", heap.heap[i]->rank);
        }
        printf("\n");
    }
}

    for(int i = 0; i < CHARS; i++) {
        struct CharNode *node = CharHeapPop(&heap);
        printf("Popped CharNode: %d %c\n", node->rank, node->ch);

        for(int i = 1; i <= heap.size; i++) {
            printf("%c", heap.heap[i]->ch);
            // printf("%d ", heap.heap[i]->rank);
        }
        printf("\n");
    }

    return 0;
}

