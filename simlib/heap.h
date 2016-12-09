/*
 * heap.h
 *
 *  Created on: 13 mar 2016
 *      Author: Enrico
 */

#ifndef HEAP_H_
#define HEAP_H_

/*
 * List.h
 *
 *  Created on: 05 mar 2016
 *      Author: PC
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <limits.h>
#include "utilities.h"
#include "list.h"

typedef sList array;



struct Heap
{
	char * id;
	sList *** Index;
	int K;
	int N;
	int M;
};

typedef struct Heap sHeap;


#define WRITE_FAILURE -1
#define READ_FAILURE -2

/* Templates */
sList *		addEventToHeap(sHeap *heap, sList *, double precision);

void 		ce_heapsort(sHeap *,double );

sHeap *    	delEventFromHeap(sHeap *heap, int, double precision);
void 		display(sHeap *);
void 		displayHeap(sHeap *heap );
void 		downheap(sHeap *, int , int, double );

void 		exch(sHeap * pq, int i, int j);

void 		freeHeap(sHeap *);

sList * 	get(sHeap *, int );

sHeap * 	initializeHeap(int, int);

boolean 	less(sHeap * pq, int i, int j, double precision);
void 		linearTransformationHeap(sHeap *, double , double  );

void 		printPos(int *, int );

sList * 	readHeap(sHeap *, int);
sList *  	readHeapNode(sHeap *, int );

int 		search(sHeap *heap, double key, double precision);
void 		shrink(sHeap *, int );
void 		sink(sHeap *, int , int, double );
void 		swap(double*, double *);

void 		upHeap(sHeap *,  double );

int 		writeHeap(sHeap *, int, sList *);


#endif /* HEAP_H_ */
