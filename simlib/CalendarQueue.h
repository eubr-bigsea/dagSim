/*
## Copyright 2017 Enrico Barbierato <enrico.barbierato@polimi.it>
## Copyright 2017 Marco Gribaudo <marco.gribaudo@polimi.it>
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
##     http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.
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
