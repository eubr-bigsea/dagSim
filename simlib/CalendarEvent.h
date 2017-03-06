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


#ifndef CALENDAR_EVENT_H_
#define CALENDAR_EVENT_H_


#include "utilities.h"
#include "heap.h"
#include "CalendarQueue.h"
#include "list.h"




struct Parameters
{
	int choice;
	double epsilon;
	int numberOfBuckets;
	int bucket_width;
	int size; // Maximum events (currently an int, but it will be declared as double in future versions
	int K, M; // Heap parameters

};

typedef struct Parameters sParameters;
struct CalendarEvent
{
	union
	{
		sList * list;
		sHeap * heap;
		sCalendarQueue * cq;
	} datastructures;
	/* Precision for comparing doubles */
	double precision;
	/* sEventCalendar identifier */
	//char *id;
	int currentChoice;
	int howmany;// Current CalendarEvent capacity
};

typedef struct CalendarEvent sCalendarEvent;


// Templates



sCalendarQueue * 	addCalendarQueue(char *, int , int , double );
sList * 			addEvent(sCalendarEvent *, sList * );

sCalendarEvent * 	createCE(struct Parameters *  );
sList * 			createEvent(double);
sList * 			createEventW(double, int);

sList * 			destroyEvent(sCalendarEvent *, sList *, boolean);
void				DelEventFromCalendarQueue(sCalendarQueue *, sList * );
void 				displayCE(sCalendarEvent *);


void 				freeEvent(sList **);
void 				freeCalendarEvent(sCalendarEvent *);

sList * 			getCEFirst(sCalendarEvent *);
double 				getEventTimestamp(sList * );

void 				linearTimeTransformation(sCalendarEvent *, double, double, double);



void				popEvent(sCalendarEvent *, sList **);

void 				readCalendarQueue(sCalendarQueue *pointer);
void				RPop(sAuxlists *, boolean, sList **);

sList * 			setEventTimestamp(sList *, double );


#endif


