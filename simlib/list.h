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


#ifndef LIST_H_
#define LIST_H_

#include "utilities.h"


#define HEAD 0
#define TAIL 1




/*
 * List
 * ======
 * It includes:
 * - An Id;
 * - A timestamp;
 * - A pointer to the next record.
 */
struct List
{
	double T;
	int W;
	int threshold;
	boolean updated;
	void *data;
	struct List *next;
	struct List *previous;
};
typedef struct List sList;
// The element corresponding to the event
typedef sList  item;

struct Auxlists
{
	int N1;
	sList * auxF, *auxL;
	sList *pointer;
};


typedef struct Auxlists sAuxlists;

#define DEFAULT_BUCKET_WIDTH 1000
#define DEFAULT_NUMBER_OF_BUCKETS 100000

/*
 * Function templates
 */

sList * 		addEventToList(sList **, sList *, double );
sList * 		addToAux(sAuxlists *,  sList * , boolean );

sAuxlists * 	createAuxLists();

void 			delEventFromList(sList **,  sList * );
sList * 		delItemAux(sAuxlists *,  boolean );

sList * 		extractFromList(sList *);

void 			freeAuxList(sAuxlists * );
void 			freeList(sList *);

double 			getHowmanyAux(sAuxlists *);

void 			linearTransformationList(sList *, double, double);

sList *			popAux(sAuxlists *, int);

void 			readEvent(sList *);
void 			readEventW(sList *);
void 			readList(sList *);
void 			readListW(sList *);

void 			updateThresholds(sList * );

#endif /* LIST_H_ */
