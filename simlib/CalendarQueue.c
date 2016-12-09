/*
 * CalendarQueue.c

 *
 *  Created on: 28 mar 2016
 *      Author: Enrico
 */



#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <string.h>


#include "CalendarQueue.h"

/*
 * 		DelEventFromCalendarQueue
 *
 * 		It deletes an Event from the bucket in the CalendaQueue
 *
 * 		Input:
 * 			- Pointer to a CalendarQueue
 * 			- Pointer to the node to be deleted
 * 			- A flag to specify if the node has to be destroyed or just disconnected
 * 		Output:
 * 			- Updated pointer to CalendarQueue
 */
void DelEventFromCalendarQueue(sCalendarQueue *cq, sList * node)
{

		assert(node!=NULL);
		// Determine where the event to be deleted should be
		int index = floor(node->T/cq->bucketSize);
		if (index < cq->numberOfBuckets)
			delEventFromList(&cq->buckets[index], node);
		else sError(BUCKET_OVERFLOW, "DeleteEventFromCalendarQueue", 0);


}

void bucketTransform(sCalendarQueue *cq, sList *pointer, double alfa, double beta, double precision, int min, int max)
{
	double temp;

	while (pointer)
			{
				temp = pointer->T;
				if (!pointer->updated)
				{
						temp = temp * alfa + beta;

							sList * next = pointer->next;
							DelEventFromCalendarQueue(cq, pointer);
							pointer->updated = true;
							pointer->T = temp;
							addEventToCalendarQueue(cq, pointer, precision);
							pointer = next;
				}
				else
				{

					pointer->updated = false;
					pointer = pointer->next;
				}
			}
}


void linearTransformationCalendarQueue(sCalendarQueue *pointer, double alfa, double beta, double precision)
{
	int index;
	int min = 0;
	int max = pointer->bucketSize-1;

	for(index=0;index<pointer->numberOfBuckets;index++)
	{
		bucketTransform(pointer, pointer->buckets[index], alfa, beta, precision,  min, max);
		min = min + pointer->bucketSize;
		max = max + pointer->bucketSize;

	}


}


/*
 * 		freeCalendarQueue
 *
 * 		It deallocates the memory for the CalendarQueue
 *
 * 		Input: a pointer to the Calendar Queue
 */


void freeCalendarQueue(sCalendarQueue *pointer)
{
	int index;

	for(index=0;index<pointer->numberOfBuckets;index++)
		{
			freeList(pointer->buckets[index]);
		}
}



/*
 * 			addCalendarQueue
 * 			================
 *
 * 			- It creates the CalendarQueue record;
 * 			- It creates NUMBER_OF_BUCKETS buckets;
 * 			- It initializes the min/max arrays, each one of BUCKET_SIZE size
 *
 *		    Parameters:
 *		    ==========
 *
 *		    Input
 *		    =====
 *		    - Pointer to the first element of the list;
 *		    - The identifier
 *
 *		    Output
 *		    ======
 *		    - The pointer to the first record of the list
 *
 */
sCalendarQueue * addCalendarQueue(char *id, int numberOfBuckets, int bucketSize, double precision)
{
	int index;


	sCalendarQueue * new = (sCalendarQueue *)malloc(sizeof (sCalendarQueue));
	if (new == NULL) sError(OUT_OF_MEMORY, "addCalendarQueue:CalendarQueue", 0);

	new->buckets = (sList **)malloc(sizeof(sList *)*numberOfBuckets);
	if (new->buckets == NULL) sError(OUT_OF_MEMORY, "addCalendarQueue:CalendarQueue:new->buckets", 0);

	for (index=0; index<numberOfBuckets;index++)
	{
		new->buckets[index] = (sList *)malloc(sizeof(sList ));
		if (new->buckets[index]==NULL) sError(OUT_OF_MEMORY,"addCalendarQueue:new->buckets[index]",index);
		new->buckets[index] = NULL;
	}


	new->id = (char *)malloc(256);
	if (new->id==NULL) sError(OUT_OF_MEMORY,"addCalendarQueue:new->id",0);
	strcpy(new->id, id);

	new->numberOfBuckets = numberOfBuckets;
	new->bucketSize = bucketSize;
	new->position = -1;


	return new;
}

/*
 * 			addItemToCalendarQueue
 * 			======================
 *
 * 			- It adds an item to the CalendarQueue by choosing the right bucket
 *
 * 			Input
 * 			=====
 * 			- The pointer to the CalendarQueue record;
 * 			- The item to be added
 *
 * 			Output
 * 			======
 * 			A pointer to sList
 */

sList * addEventToCalendarQueue(sCalendarQueue *calendarqueue, sList *node, double precision)
{
	// Find the right Bucket

	int index = floor(node->T/calendarqueue->bucketSize); //printf("index =%lf (%d) for timestamp %lf\n", floor(timestamp/BUCKET_SIZE), index, timestamp);
	if (index == -1) sError(BUCKET_UNDERFLOW, "addItemToCalendarQueue", NO_REASON);
	if (index >=calendarqueue->numberOfBuckets) sError(BUCKET_OVERFLOW, "addItemToCalendarQueue", calendarqueue->numberOfBuckets);

	return(addEventToList(&calendarqueue->buckets[index],node, precision));

}



/*
 * 			readCalendarQueue
 * 			=================
 *
 * 			It prints the content of the CalendarQueue and the content of its buckets
 *
 * 			Input
 * 			=====
 * 			- The pointer to the first record of the CalendarQueue
 *
 * 			Output
 * 			======
 * 			nothing
 */

void readCalendarQueue(sCalendarQueue *pointer)
{

	int index=0;
	int min = 0;
	int max = pointer->bucketSize-1;


	printf("\nCalendar Queue name:%s\n", pointer->id);


	for(index=0;index<pointer->numberOfBuckets;index++)
	{

		if (pointer->buckets[index]!=NULL)
		{
			printf("Bucket %d [%d - %d] : -> ", index, min, max);
			readList(pointer->buckets[index]);
			printf("\n");
		}
		min = min + pointer->bucketSize;
		max = max + pointer->bucketSize;
	}
}
