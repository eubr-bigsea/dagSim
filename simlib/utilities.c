/*
 * utilities.c
 *
 *  Created on: 06 apr 2016
 *      Author: Enrico
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "utilities.h"
/*
 * doubleCompare
 * Compare two double, returning:
 *
 * 0  if a = b
 * -1 if a < b
 * 1  if a > b
 */
int doubleCompare(double a, double b, double epsilon)
{


	  if (((a - epsilon) < b) &&
	      ((a + epsilon) > b))
	   {
	    return 0;
	   }
	  else
	   {
	    if (a > b) return 1; else return -1;
	   }

}



unsigned int randr(unsigned int min, unsigned int max)
{
       double scaled = (double)rand()/RAND_MAX;

       return (max - min +1)*scaled + min;
}

void sWarning(const int warningcode, char * where, int reason1, int reason2)
{
#ifdef CE_DEBUG

	printf("Warning:");
	switch(warningcode)
	{
		case RESIZE_CQ:
			printf("The current number of buckets(%d) is not correct for %d events.\n", reason1, reason2);
			printf("You need to force the calendar queue resize operation.\n");
			break;
		case REINTRODUCE_DESTROYED_NODE:
			printf("The node you are inserting was destroyed before. Add ignored.\n");
			break;
		case EMPTY_BUCKET:
			printf("Empty list or bucket in %s\n",where);
			break;
		case EMPTY_HEAP:
			printf("The Heap is empty in %s\n",where);
			break;
	}

#endif
}

void sError(const int errorCode, char * where, int reason)
{
	switch(errorCode)
	{
	case OUT_OF_MEMORY:
		printf("Malloc failure in %s\n", where);
		break;
	case BUCKET_UNDERFLOW:
			printf("0 is not admissible value for a bucket index in %s\n", where);
			break;
	case BUCKET_OVERFLOW:
			printf("item too big for Calendar Queue. Allocate more buckets (currently allocated:%d) in %s or change bucket size\n", reason, where);
			break;
	case UNKNOWN_CASE:
			printf("Unknown case in switch statement: see %s\n", where);
			break;
	case NOT_SUPPORTED:
			printf("Unrecognized Data Structure: see %s\n", where);
			break;
	}
	exit(-1);
}
