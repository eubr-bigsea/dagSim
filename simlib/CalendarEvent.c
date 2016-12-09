/*
 * CalendarEvent.c
 *
 *  Created on: 02 mag 2016
 *      Author: Enrico
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include "CalendarEvent.h"

sList * createEventW(double timestamp, int weight)
{
	{
		sList *node = (sList *)malloc(sizeof(sList));
		if (node==NULL) sError(OUT_OF_MEMORY, "createEvent", 0);

		node->T = timestamp;
		node->W = weight;
		return node;
	}
}

sList * createEvent(double timestamp)
{
	sList *node = (sList *)malloc(sizeof(sList));
	if (node==NULL) sError(OUT_OF_MEMORY, "createEvent", 0);

	node->T = timestamp;
	node->updated = false;

	return node;
}

sList * setEventTimestamp(sList *node, double timestamp)
{
	if (node!=NULL) node->T = timestamp;

	return node;
}

double getEventTimestamp(sList * node)
{
	if (node!=NULL) return node->T; else return -1;
}

double ceCurrentCapacity(sCalendarEvent *ce)
{
	return ce->howmany;
}

/*
 * 		freeCalendarEvent
 *
 * 		It releases all the data structures used
 * 		(list, Heap or Calendar Queue)
 *
 * 		Input: a pointer to the CalendarEvent structure
 */
void freeCalendarEvent(sCalendarEvent *ce)
{
	switch(ce->currentChoice)
	{
		case LIST:
			freeList(ce->datastructures.list);
			break;
		case HEAP:
			freeHeap(ce->datastructures.heap);
			break;
		case CALENDAR_QUEUE:
			freeCalendarQueue(ce->datastructures.cq);
			break;
		default:
			sError(UNKNOWN_CASE, "freeCalendarEvent", 0);
	}

	free(ce);
}






/*
 * 		createCE
 *
 * 		It creates a CalendarEvent data structure, which can be one of
 * 		List, Heap or CalendarQueue. According to the user choice, the corresponding
 * 		data struture is created (i.e. the memory is allocated).
 *
 * 		Input: a Parameter structure including the parameters for the CalendarEvent
 * 		Output: a pointer to the CalendarEvent data structure
 */
sCalendarEvent * createCE(sParameters *pars )
{

	sCalendarEvent *ce = (sCalendarEvent *) malloc(sizeof(sCalendarEvent));
	if (ce == NULL) sError(OUT_OF_MEMORY, "createCE: malloc", 0);


	 ce->currentChoice = pars->choice;// It can be one of: LIST, HEAP or CALENDAR_QUEUE
	 ce->precision = pars->epsilon;
	 ce->howmany = 0;


	switch (pars->choice)
			{
			case LIST:
				ce->datastructures.list = NULL;
				break;
			case HEAP:
				ce->datastructures.heap = initializeHeap(pars->K, pars->M);
				break;
			case CALENDAR_QUEUE:
				ce->datastructures.cq = addCalendarQueue("TEST", pars->numberOfBuckets, pars->bucket_width, ce->precision);
				break;
			default:
				break;
			}
	return ce;
}



/*
 * 		addEvent
 *
 * 		It adds an event on the CalendarEvent data structure
 *
 * 		Input: a pointer to the CalendarEvent
 * 		A pointer to an Event
 */
sList * addEvent(sCalendarEvent *ce, sList *node)
{
    if (node==NULL)
    {
    	sWarning(REINTRODUCE_DESTROYED_NODE,"",0,0);
    	return NULL;
    }

    ce->howmany++;
#ifdef CE_DEBUG
    printf("[add %lf] ", node->T);
#endif

    /* Update threshold */
    if (ce->currentChoice!=HEAP)
    {
    	if (ce->howmany>0) node->threshold = node->threshold+node->W;else node->threshold = node->W;
    }

	switch (ce->currentChoice)
	{
		case LIST:
			return(addEventToList(&ce->datastructures.list, node, ce->precision));
			break;
		case HEAP:
			return addEventToHeap(ce->datastructures.heap, node, ce->precision);
			break;
		case CALENDAR_QUEUE:
			ce->datastructures.cq->position = -1;
			return(addEventToCalendarQueue(ce->datastructures.cq, node, ce->precision));
			break;
		default:
			sError(5, "addEvent", 0);
		break;
	}
	printf("Unrecognized error in addEvent\n");
	return NULL;
}




/* DisplayCE
 *
 * 		It displays the content of the CalendarEvent data structure
 *
 * 		Input: A pointer to the CalendarEvent data structure
 * 		Output: none
 */
void displayCE(sCalendarEvent *pointer)
{

#ifdef CE_DEBUG
	switch (pointer->currentChoice)
		{
		case LIST:
			 // Display the list
			readList(pointer->datastructures.list);
			break;
		case HEAP:
			displayHeap(pointer->datastructures.heap);
			break;
		case CALENDAR_QUEUE:
			readCalendarQueue(pointer->datastructures.cq);
				break;
		default:
			break;
		}
#endif
}

/*
 * 	PopEvent
 *
 * 	It performs a "pop" operation on the CalendarEvent
 * 	(list, Heap or CalendarQueue) data structure
 *
 * 	Input:
 * 		- a pointer to a Calendar Event
 * 		- a flag to REMOVE or DESTROY the first element
 *
 * 	Output:
 * 		- A pointer to a List (the "popped" item)
 */

void popEvent(sCalendarEvent *ce, sList **node)
 {

	int index, base;
	#ifdef CE_DEBUG
		printf("[pop] ");
	#endif
		ce->howmany--;
		switch(ce->currentChoice)
		{
		case LIST:
			*node = ce->datastructures.list;
			ce->datastructures.list = ce->datastructures.list->next;
			//destroyEvent(ce,  ce->datastructures.list, REMOVE);
			break;
		case HEAP:
			*node = readHeapNode(ce->datastructures.heap, 1);
			ce->datastructures.heap = delEventFromHeap(ce->datastructures.heap, 1, ce->precision);

			break;
		case CALENDAR_QUEUE:
			// Determine the smallest index bucket containing the events
			if ((ce->datastructures.cq->position == -1)||(ce->datastructures.cq->buckets[ce->datastructures.cq->position] == NULL))
			{
				if (ce->datastructures.cq->position == -1) base = 0;else base = ce->datastructures.cq->position;
				for (index = base; index < ce->datastructures.cq->numberOfBuckets;index++)
					if (ce->datastructures.cq->buckets[index] != NULL)
					{
						ce->datastructures.cq->position = index;
						break;
					}
			} else index = ce->datastructures.cq->position; // No need to update the value of position since bucket[position] is not empty
			(*node) = ce->datastructures.cq->buckets[index];
			//destroyEvent(ce,  ce->datastructures.cq->buckets[index], REMOVE);
			ce->datastructures.cq->buckets[index] = ce->datastructures.cq->buckets[index]->next;
			break;
		}

 }


void linearTimeTransformation(sCalendarEvent *ce, double alfa, double beta, double precision)
{
	switch(ce->currentChoice)
	{
	case LIST:
		linearTransformationList(ce->datastructures.list, alfa, beta);
		break;
	case HEAP:
		linearTransformationHeap(ce->datastructures.heap, alfa, beta);
		break;
	case CALENDAR_QUEUE:
		linearTransformationCalendarQueue(ce->datastructures.cq, alfa, beta, precision);
		break;
	}
}

void RPop(sAuxlists * aux, boolean flag, sList ** node)
 {
	#ifdef CE_DEBUG
		printf("[RPop ");
	#endif
		int r;
		int index;

		aux->N1--;

		sList *pointer = aux->auxF;

		switch(flag)
		{
			case UNIFORM:

				r = ( rand() % aux->N1 )+1;
				*node = popAux(aux, r);
#ifdef CE_DEBUG
				printf("(U){%d} %lf] ",r, (*node)->T);
#endif
				break;
			case WEIGHT:
				index =1;
				r = ( rand() % aux->auxL->threshold )+1;
				while (pointer!=NULL)
					if (r < pointer->threshold)
					{
						*node = popAux(aux, index);
#ifdef CE_DEBUG
						printf("(W){%d} %lf] ",r, (*node)->T);
#endif
						break;
					}
					else
						{
							index++;
							pointer = pointer->next;
						}
				break;
		}

 }




void freeEvent(sList ** node)
{
	if (*node!=NULL) free(*node);
	*node = NULL;
}


sList * getCEFirst(sCalendarEvent *ce)
{

	int index;
	switch(ce->currentChoice)
	{
	case LIST:
		return ce->datastructures.list;
		break;
	case HEAP:
		//return ce->datastructures.heap->array[1];
		break;
	case CALENDAR_QUEUE:
		for (index = 0; index < ce->datastructures.cq->numberOfBuckets;index++)
			if (ce->datastructures.cq->buckets[index]!=NULL) break;
		return ce->datastructures.cq->buckets[index];
	}

	return NULL;
}


boolean isCEEmpty(sCalendarEvent *ce)
{
	return(ce->howmany==0);
}


/*
 * 		destroyEvent
 *
 * 		It destroys (i.e. performs a "free" function)
 * 		or removes (i.e. without using a "free" function) an event from the Calendar Event
 * 		by calling the procedure corresponding to the data structure used (list, Heap or
 * 		CalendarQueue)
 *
 * 		Input:
 * 			- A pointer to the CalendarEvent
 * 			- The event
 * 			- A flag (to use or not use the "free" function)
 */
sList * destroyEvent(sCalendarEvent *ce, sList *node, boolean destroy)
{

	if (node==NULL)
	{
		printf("Warning: null node passed to destoyEvent\n");
		return NULL;
	}
#ifdef CE_DEBUG
	if (destroy) printf("[destr "); else printf("[del ");
	printf("%lf] ", node->T);
#endif


	ce->howmany--;
	switch (ce->currentChoice)
	{
		case LIST:
			delEventFromList(&ce->datastructures.list, node);
			break;
		case HEAP:
			delEventFromHeap(ce->datastructures.heap, search(ce->datastructures.heap, node->T,ce->precision), ce->precision);
			break;
		case CALENDAR_QUEUE:
			DelEventFromCalendarQueue(ce->datastructures.cq, node);
			break;
		default:
			sError(UNKNOWN_CASE, "destroyEvent", 0);
		break;
	}
	if (destroy==DESTROY)
			{
				free(node);
				node = NULL;
			}

	return node;

}








