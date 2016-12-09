#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "matrix/Matrix.h"
#include "ssLib.h"

//////////// STATE SPACE ENCODING FUNCTIONS ///////////////

// creates and returns a state space of <levels> different components
StateSpace *CreateStateSpace(int levels) {
	StateSpace *out;
	
	out = malloc(sizeof(StateSpace));
	out->levels = levels;
	out->size = 0;
	out->root = NULL;
	out->decTab = NULL;
	
	return out;
}

// creates and returns a state space element with the given value
SSel *CreateSSel(int val) {
	SSel *out;
	
	out = malloc(sizeof(SSel));
	out->parent = NULL;
	out->sibling = NULL;
	out->firstChild = NULL;
	out->size = 1;
	out->val = val;

	return out;
}

// destroy a state space element and all its descendant and relatives
void DestroySSel(SSel *ssel) {
	if(ssel->firstChild != NULL) {				// destory childerns (if any)
		DestroySSel(ssel->firstChild);
	}
	if(ssel->sibling != NULL) {					// destroy siblings (if any)
		DestroySSel(ssel->sibling);
	}
	free(ssel);									// destroy this element
}

// destroy a state space
void DestroyStateSpace(StateSpace *ss) {
	if(ss->size > 0) {				// destroy the states, if any
		DestroySSel(ss->root);
	}
	if(ss->decTab != NULL) {
		free(ss->decTab);
	}
	free(ss);
}

// finds the id of a state, or return -1 if it does not exists
int FindState(StateSpace *ss, int *vals) {
	int i;
	SSel *lev, *sib;	// pointers to the level and to the current sibling
	
	if(ss->size == 0) {	// if the state space is empty, return -1
		return -1;
	}
	
	sib = lev = ss->root;
	for(i = 0; i < ss->levels; i++) {	// look for all the levels
		for(sib = lev; sib != NULL; sib = sib->sibling) {
			if(sib->val == vals[i]) {	// found a matching level
				break;
			}
		}
		if(sib == NULL) {	// not found at this level, so state does not exists
			return -1;
		}
		lev = sib->firstChild;
	}
	return sib->size;		// the state id is encoded in the size field
}

// creates a new state and returns its id
int NewState(StateSpace *ss, int *vals) {
	int out;
	int i;
	SSel *lev, *sib;	// pointers to the level and to the current sibling
	SSel *last;			// last element in the search: to add at the end of the list
	
	out = ss->size; // the new state will have the next ID
	if(ss->size == 0) {	// the first state is inserted in a different way
		ss->root = CreateSSel(vals[0]);				// creates the root
		lev = ss->root;
		for(i = 1; i < ss->levels; i++) {			// creates all the levels
			lev->firstChild = CreateSSel(vals[i]);
			lev->firstChild->parent = lev;
			lev = lev->firstChild; 
		}
	} else {			// not the first state
		sib = lev = ss->root;	// find the first level that does not exists
		for(i = 0; i < ss->levels; i++) {	// look for all the levels
			for(sib = lev; sib != NULL; sib = sib->sibling) {
				last = sib;
				if(sib->val == vals[i]) {	// found a matching level
					break;
				}
			}
			
			if(sib == NULL) {	// not found at this level, so build everthing from this point
				break;
			}
			lev = sib->firstChild;
		}
		// increase the size of the upper nodes
		for(sib = lev->parent; sib != NULL; sib = sib->parent) {
			sib->size++;
		}
		// adds a new node to the current level
		last->sibling = CreateSSel(vals[i]);
		last->sibling->parent = lev->parent;
		lev = last->sibling;
		// creates all the levels below the newly created one
		for(i = i+1; i < ss->levels; i++) {			// creates all the levels
			lev->firstChild = CreateSSel(vals[i]);
			lev->firstChild->parent = lev;
			lev = lev->firstChild; 
		}
		
	}

	lev->size = out;			// sets the number of the state
	
	ss->size++;		// increases the states of the model
	
	return out;
}

// looks for a state in the state space: if found, retunrs its id, otherwise creates a new one
int GetState(StateSpace *ss, int *vals) {
	int out;
	
	out = FindState(ss, vals);	// looks if the state exists
	if(out >= 0) {				// if found,
		return out;				// returns its id
	} else {
		return NewState(ss, vals);	// otherwise creates a new id
	}
}

// Helper function for recursive state space finalization
void RecursiveFinalization(StateSpace *ss, SSel *ssel, int level, int *id) {
	if(level == ss->levels-1) {			// for the leaf
		ss->decTab[ssel->size] = *id;		// put the id in the decoding tab
		*id = (*id) + 1;				// go to the next state number
	}
	if(ssel->firstChild != NULL) {				// destory childerns (if any)
		RecursiveFinalization(ss, ssel->firstChild, level+1, id);
	}
	if(ssel->sibling != NULL) {					// destroy siblings (if any)
		RecursiveFinalization(ss, ssel->sibling, level, id);
	}
}
// finalizes the state space. This function must be called at the end of
// the state space generation. After this, the deconding of the state space
// will be available
void FinalizeStateSpace(StateSpace *ss) {
	int id;

	id = 0;
	ss->decTab = calloc(ss->size, sizeof(int));
	RecursiveFinalization(ss, ss->root, 0, &id);
}

// returns the definition of the state with a given number
// returns 0 if the state does not exists, 1 otherwise
int DecodeState(StateSpace *ss, int id, int *vals) {
	int i, wid;
	SSel *lev, *sib;	// pointers to the level and to the current sibling
	
	if(ss->decTab == NULL) {
		fprintf(stderr, "ERROR: state space has not been finalized. Cannot DecodeState()\n");
		return 0;
	}
	
	if(id >= ss->size) {	// if the id is greater than the number of states
		fprintf(stderr, "ERROR: looking for a state out of bound (%d>=%d). Cannot DecodeState()\n",
			id, ss->size);
		return 0;
	}
	wid = ss->decTab[id];				// converts the id in the leaf number
	lev = ss->root;
	for(i = 0; i < ss->levels; i++) {	// look for all the levels
		for(sib = lev; sib != NULL; sib = sib->sibling) {
			if(i < ss->levels-1) {	// for all, but the last level
			 	if(wid < sib->size) {	// check if it is in this sibling sub-tree
			 		vals[i] = sib->val;
					break;
				} else {
					wid -= sib->size;
				}
			} else {	// for the last level
				if(wid == 0) {		// check this is the right state
			 		vals[i] = sib->val;
			 		break;
				} else {
					wid--;
				}
			}
		}
		if(sib == NULL) {	// not found at this level, so state does not exists
			fprintf(stderr, "ERROR: state tree malformed. Cannot DecodeState()\n");
			return 0;
			return 0;
		}
		lev = sib->firstChild;
	}
	
	return 1;
}








//////////////// Model generation functions

// Creates a new next state for a transition
// The <stDef> array, encodes the state. It must be an array that will be used
// and deallocated by the procedure
OutState *CreateNextState(int *stDef, double rate, OutState *next) {
	OutState *out;
	
	out = malloc(sizeof(OutState));
	
	out->stDef = stDef;
	out->rate = rate;
	out->next = next;
	
	return out;
}

// Destroy a list of state transitions. If next it is not null, it recursively destroy
// the element of the list
void DestroyOutState(OutState *OSt) {
	// recursively clears the next state list
	if(OSt->next != NULL) {
		DestroyOutState(OSt->next);
	}
	// if there is state definition, it must be destroyed
	if(OSt->stDef != NULL) {
		free(OSt->stDef);
	}
	// destroys the object
	free(OSt);
}

// Creates a new element for the out matrix list
MatElRCR *CreateMatElRCR(int row, int col, double rate, MatElRCR *next) {
	MatElRCR *out;
	
	out = malloc(sizeof(MatElRCR));

	out->row = row;
	out->col = col;
	out->rate = rate;
	out->next = next;
	
	return out;
}

// Destroy the out matrix list. If next it is not null, it recursively destroy
// the element of the list
void DestroyMatElRCR(MatElRCR *Mlist) {
	// iteratively clears the next state list, to avoid stack overflow
	MatElRCR *el, *toDel;
	
	for(el = Mlist; el != NULL;) {
		toDel = el;
		el = el->next;
		free(toDel);
	}
}




// Create a model with the state of <levels> levels, and initial state
// described by <initialState>. The procedure gets a pointe to model definition data <M>
// The transition will be generated by function
//
// OutState *transFunc(int *st, void *M) { ... }
//
//    the parameter passed to the function are the state definition <st> and the
//    model definition that was passed in pointer <M>.
//    The function returns the out transition from state <st> encoded in the list of type
//    OutState * returned by the function. Each node of the list, contains the definition
//    of the next state, and the corresponding transition rate.
//    GenerateModelMatrix automatically frees the elements of OutState during it execution
//
// The function returns the model matrix, and the state space if <outSS> != NULL
//
Matrix *GenerateModelMatrix(int levels, 
							OutState *(*transFunc)(int *, void *),
						    int *initialState, void *M, StateSpace **outSS) {
	Matrix *out;			// the infinitesimal generator for the model
	StateSpace *SS;			// the state space of the model
	int *stDef;				// state definition array
	OutState *nextTrans;	// output transition 
	OutState *firstOSt, *lastOSt;	// list of next states to consider
	OutState *thisOSt;		// the current state
	OutState *nextOSt;		// the current state
	MatElRCR *outMatBld, *ombIter;	// structure to build the output matrix
	int i, currStId, nextStId, stCount;

	// creates the state space
	SS = CreateStateSpace(levels);	
	
	// creates and inserts the initial state		
	stDef = calloc(levels, sizeof(int));	
	for(i = 0; i < levels; i++) {
		stDef[i] = initialState[i];
	}
	lastOSt = firstOSt = CreateNextState(stDef, 0.0, NULL);

	// visits the states
	outMatBld = NULL;
	stCount = 1;				// counts the visited states
	while(firstOSt != NULL) {
		// get the first unvisited state
		thisOSt = firstOSt;
		stDef = thisOSt->stDef;
		// get the id of the considered state
		currStId = GetState(SS, stDef);
		// generate the next satates
		nextTrans = transFunc(stDef, M);	
		// inserts the generated states in the array to build the matrix
		while(nextTrans != NULL) {
			nextOSt = nextTrans;			// removes the element from the next state list
			nextTrans = nextTrans->next;
			nextOSt->next = NULL;

			nextStId = GetState(SS, nextOSt->stDef);	// looks for the state id
			
			// insert in the out list
			outMatBld = CreateMatElRCR(currStId, nextStId, nextOSt->rate, outMatBld);

			if(nextStId == stCount) {	// this state has been introduced now
				lastOSt->next = nextOSt;	// put it at the end of the list
				lastOSt = nextOSt;
				stCount++;					// increase the number of states
			} else {						// if this state was already introduced
				DestroyOutState(nextOSt);	// it can be destroyed
			}
		}
				
		// move to the next state
		firstOSt = thisOSt->next;
		thisOSt->next = NULL;
		DestroyOutState(thisOSt);
	}

	// creates the matrix from the list
	out = CreateMatrixEnc(SS->size, SS->size, ME_SPARSE_C);
	for(ombIter = outMatBld; ombIter != NULL; ombIter = ombIter->next) {
		WriteMat(out, ombIter->row, ombIter->col, ombIter->rate);
	}

	DestroyMatElRCR(outMatBld);	// destroys the element list

	// returns or destroy the state space if not requested
	if(outSS != NULL) {
		FinalizeStateSpace(SS);
		*outSS = SS;
	} else {
		DestroyStateSpace(SS);
	}
	
	return out;
}



















