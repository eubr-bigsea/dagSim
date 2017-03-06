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


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "list.h"

void linearTransformationList(sList *pointer, double alfa, double beta)
{
	while (pointer)
	{
		pointer->T = pointer->T*alfa+beta;
		pointer = pointer->next;
	}
}

/*
 * 		delEventFromList
 *
 * 		It destroys (or simply disconnect) an Event from the List/Bucket
 *
 * 		Input: a pointer to the List/Bucket structure
 * 			   a pointer to the node including the Event to be destroyed or disconnected
 * 			   a flag specifying if the node has t be destroyed (with a "free" function) or
 * 			   just disconnected from the List/Bucket
 */
void delEventFromList(sList **first , sList *node)
{


	  if(*first == NULL || node == NULL)
	    return;

	  /* If node to be deleted is the first node */
	  if(*first == node) *first = node->next;

	  /* Not the last node */
	  if(node->next != NULL) (node->next)->previous = node->previous;

	  /*  Not the first node */
	  if(node->previous != NULL) (node->previous)->next = node->next;
}


void freeAuxList(sAuxlists * first)
{
	sList *temp;

	if (first->auxF!=NULL)
	{
		while (first->auxF->next!=NULL)
		{
			temp = first->auxF->next;
			free(first->auxF);
			first->auxF = temp;
		}
		free(first->auxF);
	}

	free(first);
}

double getHowmanyAux(sAuxlists * first)
{
	if (first == NULL) return (0);
	else
	return first->N1;
}


sAuxlists * createAuxLists()
{
	sAuxlists * first = (sAuxlists *)malloc(sizeof(sAuxlists));
	if (first==NULL) sError(OUT_OF_MEMORY, "createAUXLists",0);

	first->N1 = 0;
	first->auxF = NULL;
	first->auxL = NULL;
	first->pointer = NULL;
	return first;

}

sList *popAux(sAuxlists *aux, int pos)
{
	sList *pointer, *node, *prev, *next;
	int index = 1;

	pointer = aux->auxF;

	while (pointer!=NULL)
		if (index==pos) break; else {index++; pointer = pointer->next;}

	if (pointer==NULL)
	{
		printf("NULL pointer in popAuX\n");
		exit(1);
	}

	/* Copy the event */
	node = pointer;
	/* Remove the event and update the pointers */
	prev = pointer->previous;
	next = pointer->next;

	// First event
	if (prev == NULL)
		{	aux->auxF = next;
			next->previous = prev;
		}
	else
		// Last event
		if (next == NULL)
		{
			prev->next = next;
			return node; // No threshold update is necessary
		}
		else
			// The event is in the middle
		{
			prev->next = next;
			next->previous = prev;
		}

	updateThresholds(pointer);
	return node;
}

void updateThresholds(sList * pointer)
{
	pointer = pointer->next;

	while (pointer!=NULL)
	{
		if (pointer->previous == NULL) pointer->threshold =  pointer->W;
		else pointer->threshold = pointer->previous->threshold + pointer->W;
		pointer = pointer->next;
	}

}



sList * delItemAux(sAuxlists *aux, boolean mode)
{
	sList * node;
#ifdef CE_DEBUG
	printf("[del ");
	if (mode==HEAD) printf("(HEAD) ");
	else printf("(TAIL) ");
#endif
	if (aux->N1 == 1)
	{
		node = aux->auxF;
		aux->auxF = NULL;
		aux->auxL = NULL;
	}
	else
	{
	if (aux->auxF==NULL) {
		return NULL;
	}
	switch(mode)
	{
		case HEAD:
			node = aux->auxF;
			aux->auxF = aux->auxF->next;
			aux->auxF->previous = NULL;
			aux->auxF->threshold = aux->auxF->W;
			updateThresholds(aux->auxF);
			break;
		case TAIL:
			node = aux->auxL;
			aux->auxL = aux->auxL->previous;
			aux->auxL->next = NULL;
			break;
	}
	}
	aux->N1--;
	return node;
}


sList * addToAux(sAuxlists *aux, sList * new, boolean mode)
{
#ifdef CE_DEBUG
	printf("[add ");
#endif
	// Populate the struct
	new->next = NULL;
	new->previous = NULL;

	if (aux->auxF==NULL)
	{
		aux->auxF = new;
		aux->auxL = new;
	}

	else
	switch(mode)
		{
			case HEAD:
#ifdef CE_DEBUG
				printf("(H)");
#endif
				new->next = aux->auxF;
				aux->auxF->previous = new;
				aux->auxF = new;
			break;
			case TAIL:
#ifdef CE_DEBUG
				printf("(T)");
#endif
				new->next = NULL;
				new->previous = aux->auxL;
				aux->auxL->next = new;
				aux->auxL = new;
			break;
		}

	if (new->previous!=NULL) new->threshold = new->previous->threshold + new->W;
			else new->threshold = new->W;

	if (mode==HEAD) updateThresholds(aux->auxF);
#ifdef CE_DEBUG
	printf("%lf] ",new->T);
#endif
	aux->N1++;

	return new;
}



/*
 * 		readList
 * 		==========
 *
 * 		It prints the Bucket content
 *
 * 		Input
 * 		=====
 * 		- The pointer to the first record;
 *
 * 		Output
 * 		======
 *
 */

void readList(sList *pointer)
{
	if (pointer == NULL)
	{
		sWarning(EMPTY_BUCKET,"readList", 0, 0);
		return;
	}

	while (pointer!=NULL)
	{
		readEvent(pointer);
		//if (pointer->previous!=NULL) printf("(prev. %lf) ", pointer->previous->T);
		pointer = pointer->next;
	}
	printf("\n");
}

void readListW(sList *pointer)
{
	if (pointer == NULL)
	{
		sWarning(EMPTY_BUCKET,"readList", 0, 0);
		return;
	}

	while (pointer!=NULL)
	{
		readEventW(pointer);
		//if (pointer->previous!=NULL) printf("(prev. %lf) ", pointer->previous->T);
		pointer = pointer->next;
	}
	printf("\n");
}


void readEvent(sList * pointer)
{
	if (pointer==NULL) return;
	printf("%lf ", pointer->T);
}

void readEventW(sList * pointer)
{
	if (pointer==NULL) return;
	printf("%lf(%d)[%d] ", pointer->T, pointer->W, pointer->threshold);
}


/*
 * 		freeList
 *
 * 		It deallocate the memory for the list
 *
 * 		Input: A pointer to the list
 */
void freeList(sList *pointer)
{
	sList *node = pointer;
	while (node)
	{
	   sList *temp = node;
	   node = node->next;
	   free(temp);
	}
}

/*
 * 		addEventToList
 * 		==================
 *
 * 		It add an item to a Bucket, with respect to the order (<)
 *
 *
 * 		Input
 * 		=====
 * 		- The pointer to the first element of the Bucket
 * 		- The item to be added
 *
 * 		Output
 * 		======
 * 		- The updated pointer to the first item of the Bucket
 *
 */
sList * addEventToList(sList **first, sList *new, double precision)
{
		sList *prev = NULL;
		sList *pointer;
		int result;

		// Populate the struct
		new->next = NULL;
		new->previous = NULL;


		pointer = *first;
		prev = *first;

		if (*first==NULL) *first = new; // This is the first element
			else
		{		// Look for a suitable place to insert the new element
				while (pointer!=NULL)
				{
					result = doubleCompare(pointer->T,  new->T, precision);
					if (result <=0)
					{
						prev = pointer;
						pointer = pointer->next;
					}
					else break;
				}
				if (pointer==NULL) // That means that the elements are already sorted (we are at the end of the list)
				{
					prev->next = new;
					new->previous = prev;
				}
				else
					if (prev==pointer) // it means that I want to do swap (first, new)
					{
						new->next = *first;
						new->previous = (*first)->previous;
						(*first)->previous = new;
						 *first = new;

					}
					else // it means that the swap happens somewhere in the list
					{
						prev->next = new;
						new->previous = prev;

						new->next = pointer;
						pointer->previous = new;
					}
			}

		//	 free(prev);
		return new;
}







