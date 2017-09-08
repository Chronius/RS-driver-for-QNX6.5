/*
 * fifo.c
 *
 *  Created on: 5 мая 2017 г.
 *      Author: abuzarov_bv
 */
#include "fifo.h"

s8 fifo_put(fifo_t *fifo, void *data, int offset, int count) {
    int i;
    if ((FIFO_LENGTH - fifo->count) < count)
        return -1;
    for (i = offset; i < (offset + count); i++)
    {
        fifo->data[fifo->tail++] = ((u8*) data)[i];
        fifo->count++;
        if (fifo->tail == FIFO_LENGTH)
        {
            fifo->tail = 0;
        }
    }
    return 0;
}

s8 fifo_get(fifo_t *fifo, void *data, int offset, int count) {
    int i;
    if (fifo->count < count)
        return -1;
    for (i = offset; i < (offset + count); i++)
    {
        ((u8*) data)[i] = fifo->data[fifo->head++];
        fifo->count--;
        if (fifo->head == FIFO_LENGTH)
        {
            fifo->head = 0;
        }
    }
    return 0;
}

int fifo_count(fifo_t *fifo) {
    return fifo->count;
}

void fifo_init(fifo_t *fifo) {
    fifo->head = fifo->tail = fifo->count = 0;
}
