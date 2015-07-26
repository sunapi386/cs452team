#ifndef __HEAP_H
#define __HEAP_H
/**
Preprocessor generated template for heap binary heap (priority queue) structure.
*/
#define PARENT(x)   ((x) / 2)
#define RIGHT(x)    (2 * (x))
#define LEFT(x)     (2 * (x) + 1)

#define DECLARE_HEAP(heaptype, valuetype, value_func, height, comp)\
typedef struct heaptype {\
    valuetype heap[1 + (1<<height)];\
    int count;\
} heaptype;\
\
void heaptype##SiftUp(struct heaptype *heap, int idx){\
    if(idx == 1) return;\
    if(value_func(heap->heap[idx]) comp value_func(heap->heap[PARENT(idx)])){\
        valuetype tmp = heap->heap[idx];\
        heap->heap[idx] = heap->heap[PARENT(idx)];\
        heap->heap[PARENT(idx)] = tmp;\
        heaptype##SiftUp(heap, PARENT(idx));\
    }\
}\
\
void heaptype##SiftDown(struct heaptype *heap, int idx){\
    if(idx * 2 > heap->count) return;\
    if(value_func(heap->heap[RIGHT(idx)]) comp value_func(heap->heap[idx])){\
        valuetype tmp = heap->heap[idx];\
        heap->heap[idx] = heap->heap[RIGHT(idx)];\
        heap->heap[RIGHT(idx)] = tmp;\
        heaptype##SiftDown(heap, RIGHT(idx));\
    }\
    if(LEFT(idx) <= heap->count &&\
        value_func(heap->heap[LEFT(idx)]) comp value_func(heap->heap[idx])){\
        valuetype tmp = heap->heap[idx];\
        heap->heap[idx] = heap->heap[LEFT(idx)];\
        heap->heap[LEFT(idx)] = tmp;\
        heaptype##SiftDown(heap, LEFT(idx));\
    }\
}\
\
void heaptype##Push(struct heaptype *heap, valuetype value){\
    heap->heap[++heap->count] = value;\
    heaptype##SiftUp(heap, heap->count);\
}\
\
valuetype heaptype##Pop(struct heaptype *heap){\
    valuetype ret = heap->heap[1];\
    heap->heap[1] = heap->heap[heap->count--];\
    heaptype##SiftDown(heap, 1);\
    return ret;\
}

#endif
