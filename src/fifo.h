#ifndef FIFO_H
#define FIFO_H

#include <stdint.h>
#include <stddef.h>

#define FIFO_SIZE 3
#define FIFO_T uint32_t
#define FIFO_DEFAULT 0xFF

typedef struct fifo_t {
	FIFO_T data[FIFO_SIZE];
	size_t pos;
	size_t count;
} fifo_t;

void fifo_fill(fifo_t* f, FIFO_T value);
void fifo_push(fifo_t* f, FIFO_T value);
FIFO_T fifo_pop(fifo_t* f);

#endif /* FIFO_H */
