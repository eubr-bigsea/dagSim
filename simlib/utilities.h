/*
 * utilities.h
 *
 *  Created on: 14 apr 2016
 *      Author: Enrico
 */

#ifndef UTILITIES_H_
#define UTILITIES_H_
#include <stdio.h>

// Comment this line if you are debugging the application
//#define CE_DEBUG 1

typedef int boolean;

#define true 1;
#define false 0;

#define LIST 1
#define HEAP 2
#define CALENDAR_QUEUE 3

#define UNIFORM 1
#define WEIGHT 2

#define ITEMMIN -1

#define MAX_READABLE_EVENTS 50



#define REMOVE 0
#define DESTROY 1

#define VERSION "1.62"





#define NO_REASON 0
#define OUT_OF_MEMORY 1
#define BUCKET_UNDERFLOW 2
#define BUCKET_OVERFLOW 3
#define EVENTS_OVERFLOW 4
#define UNKNOWN_CASE 5
#define NOT_SUPPORTED 6

#define RESIZE_CQ 0
#define REINTRODUCE_DESTROYED_NODE 1
#define EMPTY_BUCKET 2
#define EMPTY_HEAP 3

/* Templates */

int doubleCompare(double, double, double);
void sError(const int, char * , int );
char* dtoa(double );
void sWarning(const int warningcode, char * where, int reason1, int reason2);

#endif /* UTILITIES_H_ */
