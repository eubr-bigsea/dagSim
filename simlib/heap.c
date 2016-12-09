/*
 * heap.c

 *
 *  Created on: 13 mar 2016
 *      Author: Enrico
 */


/*
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "heap.h"


void shrink(sHeap *heap, int newN)
{
	int k;

	if (newN/heap->M < heap->N/heap->M)
		for (k = newN/heap->M+1; k <= heap->N/heap->M; k++) free(heap->Index[k]);
}

void freeHeap(sHeap *heap)
{
	int i;

	 for (i = 1; i <= heap->N/heap->K; i++)
	    	free(heap->Index[i]);
}

sList * readHeap(sHeap *heap, int i)
{
	if (i>heap->N)
	{
		printf("Read out of bound: %d, heap->N=%d\n", i, heap->N);
		exit (-1);
	}

	if (i<heap->M) return heap->Index[0][i];
	else  return heap->Index[i/heap->M][i%heap->M];
}




sList * readHeapNode(sHeap *heap, int i)
{
	if (i>heap->N)
	{
		printf("Read out of bound: %d, heap->N=%d\n", i, heap->N);
		exit(-1);
	}

	if (i<heap->M) return heap->Index[0][i];
	else  return heap->Index[i/heap->M][i%heap->M];
}


void linearTransformationHeap(sHeap *heap, double alfa, double beta )
{
    int i;
    sList *node;

    for (i = 1; i <= heap->N; i++)
    {
    	node = readHeap(heap, i);
    	node->T = node->T*alfa+beta;

    }

}

/*
 * Display
 * it shows the element of the Heap
 */
void displayHeap(sHeap *heap )
{
    int i;
    if (heap->N == 0)
    {
        printf("Heap is empty \n");
        return;
    }
    printf("[");
    for (i = 1; i <= heap->N; i++)

    	printf("%lf ", readHeap(heap, i)->T);
    printf("]\n");
}/*End of display()*/




int writeHeap(sHeap *heap, int i, sList * x)
{

	if (i<heap->N)
		if (i<heap->M) heap->Index[0][i] = x;
		else heap->Index[i/heap->M][i%heap->M] = x;
	else
		if (i>heap->M*heap->K)
		{
			printf("Write out of bound: %lf, heap->N=%d\n", x->T, heap->N);
			return WRITE_FAILURE;
		}

	if (i<heap->M) heap->Index[0][i] = x;
	else
		if (heap->Index[i/heap->M]!=NULL) heap->Index[i/heap->M][i%heap->M] = x;
		else
		{

			heap->Index[i/heap->M] =  calloc((size_t)heap->M, sizeof(array));
			if (heap->Index[i/heap->M]==NULL)
			{
				printf("calloc failure in heap->Index[i/heap->M]\n");
				exit(-1);
			}
			heap->Index[i/heap->M][0] = x;
		}
	//heap->N = i+1;
	return 0;
}

sHeap * initializeHeap(int K, int M)
{


	sHeap * heap = (sHeap *)malloc(sizeof(sHeap));
	if (heap == NULL) sError(OUT_OF_MEMORY, "initializeHeap:heap", 0);

	heap->K = K;
	heap->M = M;
	heap->N = 0;

	heap->Index = NULL;
	heap->Index =  calloc((size_t)heap->K, sizeof(heap->Index));
	if (heap->Index==NULL)
	{
		printf("calloc failure: heap->Index\n");
		exit(-1);
	}

	heap->Index[0] = calloc((size_t)heap->M, sizeof(array));
	if (heap->Index[0]==NULL)
	{
		printf("calloc failure in heap->Index[0]\n");
		exit(-1);
	}

	return heap;
}



/*
 * addEventToHeap
 *
 * It inserts an element in the Heap
 * (no sorting is performed
 * the first element of the heap is the smallest)
 *  From "Algorithms in C++", R. Sedgewick
 */
sList * addEventToHeap(sHeap *heap, sList * node, double precision)
{
	heap->N++;


	writeHeap(heap, heap->N, node);
	//upHeap(heap, precision);
	ce_heapsort(heap, precision);
	return node;
}



void swap(double *a, double *b)
{
	double temp;

	temp = *a;
	*a = *b;
	*b = temp;
}

void ce_heapsort(sHeap *heap, double precision)
{
	int k;
	int N = heap->N;

	 for (k = N/2; k >= 1; k--)
		 sink(heap, N, k, precision);

	 while (N > 1)
	 {
	    exch(heap, 1, N--);
	    sink(heap, N, 1, precision);
	 }
}



/*
 *  From "Algorithms in C++", R. Sedgewick

void upHeap(sHeap *heap, double precision)
{
	double v;
	int k = heap->N;

	v = heap->array[k];
	heap->array[0]= ITEMMIN;

	while (doubleCompare(heap->array[k/2],v, precision)>-1)
	{
		//heap->array[k] = heap->array[k/2]; // Shallow copy
		heap->array[k] = heap->array[k/2]; // Deep copy
		k = k/2;
	}
	heap->array[k] = v;

}
*/




/*
 * delEventFromHeap
 *
 * It deletes an element from the Heap
 *
 * From "Algorithms in C++", R. Sedgewick
 *
 * Input:
 * Output:
 */

sHeap * delEventFromHeap(sHeap *heap, int pos, double precision)
{

	if (pos==-1) return heap;

	writeHeap(heap, pos, readHeap(heap, heap->N));
	heap->N--;
	//downheap(heap, heap->N,  pos, precision);
	ce_heapsort(heap, precision);
	return heap;
	}


int search(sHeap *heap, double key, double precision)
{
        int low = 1, high = heap->N, mid, comp;
        while(low <= high)
        {
                mid = (low + high)/2;
                comp = doubleCompare(readHeap(heap, mid)->T, key, precision);

                switch(comp)
                {
                	case -1:
                		low = mid + 1;
                		break;
                	case 0:
                		return mid;
                		break;
                	case 1:
                		high = mid-1;
                		break;
                }
        }

        return -1;
}


/*
 *  From "Algorithms in C++", R. Sedgewick
 */
void sink(sHeap *heap,int N, int k,   double precision)
{
	while (2*k <= N) {
	           int j = 2*k;
	           if (j < N && less(heap, j, j+1, precision)) j++;
	           if (!less(heap, k, j, precision)) break;
	           exch(heap, k, j);
	           k = j;
	       }
}

boolean less(sHeap * pq, int i, int j, double precision)
{
	   return doubleCompare(readHeap(pq, i)->T, readHeap(pq, j)->T, precision) < 0;

}

void exch(sHeap * pq, int i, int j)
{
	sList * temp;
	temp = readHeap(pq, i);
	writeHeap(pq, i, readHeap(pq, j));
	writeHeap(pq, j, temp);


   }

/*
void downheap(sHeap *heap,int n,  int k, double precision)
	{
		int j;
		double v;


		v = heap->array[k];
		while (k<=n/2)
		{
			j = k+k;
			if (
				(j<n)&&
				(doubleCompare(heap->array[j], heap->array[j+1], precision) > -1)) j++;
			if (doubleCompare(v,heap->array[j], precision)<0) break;
			//heap->array[k] = heap->array[j]; // Shallow copy
			heap->array[k] = heap->array[j]; // Deep copy

			k = j;
		}
		heap->array[k] = v;


	}
	*/

