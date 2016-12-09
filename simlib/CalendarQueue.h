/*
 * CalendarQueue.h
 *
 *  Created on: 28 mar 2016
 *      Author: Enrico
 */

#ifndef CALENDAR_QUEUE_H_
#define CALENDAR_QUEUE_H_


#include "list.h"
#include "utilities.h"





// Size of the bucket (number of items it can contain)



/*
 * 		CALENDAR QUEUE
 * 		==============
 */
struct CalendarQueue
{
	sList ** buckets;
	int numberOfBuckets;
	int bucketSize;
	char *id;
	int position;
};

typedef struct CalendarQueue sCalendarQueue;
/*
 * 		TEMPLATES
 * 		=========
 */
sCalendarQueue * 	addCalendarQueue(char *id, int , int , double precision);
sList * 			addEventToCalendarQueue(sCalendarQueue *, sList *, double );

void 				bucketTransform(sCalendarQueue *, sList *, double , double , double , int  , int );

int  				findBucketForItem(sCalendarQueue *, double);
void 				freeCalendarQueue(sCalendarQueue *);

void 				linearTransformationCalendarQueue(sCalendarQueue *, double, double, double);

void 				readCalendarQueue(sCalendarQueue *);


#endif
