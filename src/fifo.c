#include "fifo.h"

void fifo_fill(fifo_t* f, FIFO_T value){
	for (size_t i=0; i<FIFO_SIZE; ++i){
		f->data[i] = value;
	}
}

void fifo_push(fifo_t* f, FIFO_T value){
	f->data[(f->pos + f->count) % FIFO_SIZE] = value;
	if (f->count < FIFO_SIZE){
		++f->count;
	} else {
		f->pos = (f->pos +1) % FIFO_SIZE;
	}
}

FIFO_T fifo_pop(fifo_t* f){
	if (f->count == 0){
		return FIFO_DEFAULT;
	}
	FIFO_T value = f->data[f->pos];
	f->pos = (f->pos +1) % FIFO_SIZE;
	f->count--;
	return value;
}
