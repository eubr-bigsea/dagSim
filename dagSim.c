/*
	dagSim 1.0
	Date: 01/12/2016
*/


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "matrix/Matrix.h"
#include "luaInterface.h"
#include "distributions.h"
#include "ssLib.h"
#include "simlib/CalendarEvent.h"
#include "simlib/utilities.h"

typedef struct stage_tag {
	char *Id;		// Name of the DAG stage
	int nId;		// Numeric Id of the stage
	int Tasks;		// Number of tasks for the stage
	Distribution *Distr;	// Task duration distribution
	// Previous stages
	int nPre;					// number
	struct stage_tag **Pre;		// pointer
	char **PreId;				// id
	// Follwing stages
	int nPost;					// number
	struct stage_tag **Post;	// pointer
	char **PostId;				// id
} Stage;

typedef struct {
	int Nodes;		// number of nodes executing a job
	int Users;		// number of users sending the jobs
	
	Distribution *UThinkTimeDistr;	// Distribution of the think time
	int nStages;	// number of stages of the DAG
	Stage *Stages;	// Stages of the DAG
	
	int maxJobs;	// total number of jobs to simulate
	double confIntCoeff;	// coefficient for the confidence intervals
} Model;



// procedures to read a model from the lua file
// solve pre and post links of stages
void solvePrePostLinks(lua_State *L, Stage *S, Model *M) {
	int i, j, found;
	
	if(S->nPre > 0) {
		for(i = 0; i < S->nPre; i++) {
			found = -1;
			for(j = 0; j < M->nStages; j++) {
				if(strcmp(S->PreId[i], M->Stages[j].Id) == 0) {
					found = j;
					break;
				}
			}
			if(found == -1) {
				printf("\n Error: pre {%s} in stage {%s} not found\n", S->PreId[i], S->Id);
				exit(0);
			} else {
				S->Pre[i] = &M->Stages[j];
			}
		}
	}

	if(S->nPost > 0) {
		for(i = 0; i < S->nPost; i++) {
			found = -1;
			for(j = 0; j < M->nStages; j++) {
				if(strcmp(S->PostId[i], M->Stages[j].Id) == 0) {
					found = j;
					break;
				}
			}
			if(found == -1) {
				printf("\n Error: post {%s} in stage {%s} not found\n", S->PostId[i], S->Id);
				exit(0);
			} else {
				S->Post[i] = &M->Stages[j];
			}
		}
	}
}

// read one stage
void readStage(LuaWrapper *Lw, int nstg, Stage *S) {
	int i;
	lua_State *L = Lw->L;

	S->nId   = nstg;
	pushSubTable(L, nstg);

	S->Id    = readString (L, "name");
	S->Tasks = readRealDef(L, "tasks", 1);
	S->Distr = readDistribution(Lw, "distr");

//printf("%d\n", lua_gettop(L));	
	S->nPre  = countElements(L, "pre");
//printf("%d\t%d\n", lua_gettop(L), S->nPre);	
	if(S->nPre > 0) {
		S->Pre   = (Stage **)calloc(sizeof(Stage *), S->nPre);
		S->PreId = (char  **)calloc(sizeof(char  *), S->nPre);
		for(i = 0; i < S->nPre; i++) {
			S->PreId[i] = readVectorElString(L, "pre", i);
		}
	} else {
		S->Pre = NULL; S->PreId = NULL;
	}

//printf("%d\n", lua_gettop(L));	
	S->nPost = countElements(L, "post");
//printf("%d\t%d\n", lua_gettop(L), S->nPost);	
	if(S->nPost > 0) {
		S->Post   = (Stage **)calloc(sizeof(Stage *), S->nPost);
		S->PostId = (char  **)calloc(sizeof(char  *), S->nPost);
		for(i = 0; i < S->nPost; i++) {
			S->PostId[i] = readVectorElString(L, "post", i);
		}
	} else {
		S->Post = NULL; S->PostId = NULL;
	}
	lua_pop(L, 1);
}

// read the entire model
Model *createModel(LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	Model *M;
	int i;
	
	M = malloc(sizeof(Model));
	
	M->Nodes = readRealDef(L, "Nodes", 1);
	M->Users = readRealDef(L, "Users", 1);
	M->UThinkTimeDistr = readDistribution(Lw, "UThinkTimeDistr");
	
	// Read the stages
	M->nStages = countElements(L, "Stages");
	M->Stages = (Stage *)calloc(sizeof(Stage), M->nStages);
	lua_getfield(L, -1, "Stages");
	for(i = 0; i < M->nStages; i++) {
		readStage(Lw, i, &M->Stages[i]);
	}
	lua_pop(L, 1);
	for(i = 0; i < M->nStages; i++) {
		solvePrePostLinks(L, &M->Stages[i], M);
	}
	M->maxJobs = readRealDef(L, "maxJobs", 1000);
	M->confIntCoeff = readRealDef(L, "confIntCoeff", 1.96);
	return M;
}

// procedures to free the space allocated by a model
// free the memory used for a stage
void freeStage(Stage *S, LuaWrapper *L) {
	int i;

	free(S->Id);
	destroyDistribution(S->Distr, L);
	
	if(S->nPre > 0) {
		for(i = 0; i < S->nPre; i++) {
			free(S->PreId[i]);
		}
		free(S->Pre);
		free(S->PreId);
	}
	if(S->nPost > 0) {
		for(i = 0; i < S->nPost; i++) {
			free(S->PostId[i]);
		}
		free(S->Post);
		free(S->PostId);
	}
}
// free the memory used for the whole model
void destroyModel(Model *M, LuaWrapper *L) {
	int i;
	
	destroyDistribution(M->UThinkTimeDistr, L);
	for(i = 0; i < M->nStages; i++) {
		freeStage(&M->Stages[i], L);
	}
	free(M->Stages);
	free(M);
}



/***** Model solution ****/

// output a result line to lua
void sendResult(int row, int nvals, double *vals, lua_State *L) {
// row   => row in the output table
// nvals => number of values to be written
// vals  => values to be written
	int col;

	// creates a line containing the results
	lua_pushnumber(L, row);
	lua_newtable(L);
	
	// puts the elemnts in the line
	for(col = 0; col < nvals; col++) {
		setTableElemIdx(L, col + 1, vals[col]);
	}

	// sends the table to the results
	lua_settable(L, -3);
}

void RestartAllDistributions(Model *M, LuaWrapper *Lw) {
	int i;
	
	restartDistribution(M->UThinkTimeDistr, Lw);
	for(i = 0; i < M->nStages; i++) {
		restartDistribution(M->Stages[i].Distr, Lw);
	}
}


typedef struct {
	double Val;			// Sum of the measure
	double Val2;		// Sum of the square of the measure
	double N;			// Number of samples considered
} PerfIdx;

typedef struct {
	PerfIdx *M;		// Array of measures
	int N;			// number of measures
} PerfIdxs;

#define MEASURES_SYSTEM 4
#define M_SYS_TOT  0
#define M_SYS_WAIT 1
#define M_SYS_JOB  2
#define M_AVE_THINK_TIME	3

#define MEASURES_XSTAGE 4
#define M_XST_START 0
#define M_XST_END   1
#define M_XST_DUR   2
#define M_XST_AVE_SERV_TIME	3

#define MEAS_ID(S,T) ((S==0) ? T : (MEASURES_SYSTEM + MEASURES_XSTAGE * (S-1) + T))

PerfIdxs *createPerfIdxs(Model *M) {
	PerfIdxs *out = malloc(sizeof(PerfIdxs));
	int i;
	
	out->N = MEASURES_SYSTEM + M->nStages * MEASURES_XSTAGE;
	out->M = (PerfIdx *)calloc(sizeof(PerfIdx), out->N);
	
	for(i = 0; i < out->N; i++) {
		out->M[i].Val  = 0.0;
		out->M[i].Val2 = 0.0;
		out->M[i].N    = 0.0;
	}
	
	return out;
}

void destroyPerfIdxs(PerfIdxs *P) {
	free(P->M);
	free(P);
}

void measureAddSample(PerfIdxs *P, int sid, double val) {
	P->M[sid].Val  += val;
	P->M[sid].Val2 += val*val;
	P->M[sid].N++;
}

int makeConfInt(double *vals, int col, PerfIdxs *PI, int sid, double Zval) {
	if(PI->M[sid].N > 0) {
		vals[col]   = PI->M[sid].Val / PI->M[sid].N;
		vals[col+1] = sqrt(PI->M[sid].Val2 / PI->M[sid].N - vals[col]*vals[col]);
		vals[col+2] = vals[col] - vals[col+1] * Zval / sqrt(PI->M[sid].N);
		vals[col+3] = vals[col] + vals[col+1] * Zval / sqrt(PI->M[sid].N);
		vals[col+4] = (vals[col] > 0) ? (vals[col+3] - vals[col+2]) / vals[col] : 0.0;
	} else {
		vals[col]   = -1.0;
		vals[col+1] = -1.0;
		vals[col+2] = -1.0;
		vals[col+3] = -1.0;
		vals[col+4] = -1.0;
	}
	return 5;
}

typedef struct {
	int userId;
	int jobId;
	int phaseId;
	int taskId;
} Job;


#define STAGE_ST_WAITING   0
#define STAGE_ST_CAN_START 1
#define STAGE_ST_RUNNING   2
#define STAGE_ST_ENDED     3

// Data structure and procedures to store the definitions of the job submitted by a user
typedef struct {
	int uId;				// user id
	int jId;				// job id
	double submissionTime;	// submission time
	double startTime;		// start time
	double endTime;			// end time
	
	int stagesToStart;			// stages that still needs to be started
	int *remainingTasksXStage;	// jobs that still needs to be completed per stage (array of int, one per stage)
	int *stageState;			// state of the stages (array of int, one per stage)
	double *stageStart;			// start time for the stage
	double *stageEnd;			// end time for the stage
	sAuxlists *readyList;		// Lists of jobs that can be started
} UserJobData;

typedef struct {
	int NfreeNodes;			// number of nodes idle
	int UserLocker;			// ID of the user locking the resources
	sAuxlists *waitingJobs;	// Jobs that still needs to be executed
} NodeData;

UserJobData *createUserJobData(Model *M) {
	UserJobData *out;
	
	out = malloc(sizeof(UserJobData));
	out->remainingTasksXStage = calloc(sizeof(int), M->nStages);
	out->stageState = calloc(sizeof(int), M->nStages);
	out->stageStart = calloc(sizeof(double), M->nStages);
	out->stageEnd = calloc(sizeof(double), M->nStages);
	out->readyList = createAuxLists();
	return out;
}

NodeData *initNodeData(Model *M) {
	NodeData *ND = (NodeData *)malloc(sizeof(NodeData));
	ND->NfreeNodes = M->Nodes;
	ND->UserLocker = -1;
	ND->waitingJobs = createAuxLists();
	return ND;
}

void initUserJobData(UserJobData *J, double sT, int uId, int jId, Model *M) {
	int i;
	
	J->submissionTime = sT;
	J->uId = uId;
	J->jId = jId;
	J->stagesToStart = M->nStages;
	for(i = 0; i < M->nStages; i++) {
		J->remainingTasksXStage[i] = M->Stages[i].Tasks;
		J->stageStart[i] = -1.0;
//printf("%d %d\n", M->Stages[i].nPre, M->Stages[i].nPost);
		if(M->Stages[i].nPre == 0) {
			J->stageState[i] = STAGE_ST_CAN_START;
			J->stagesToStart--;
		} else {
			J->stageState[i] = STAGE_ST_WAITING;
		}
	}
}

void destroyUserJobData(UserJobData *J) {
	free(J->remainingTasksXStage);
	free(J->stageState);
	free(J->stageStart);
	free(J->stageEnd);
	freeAuxList(J->readyList);
	free(J);
}

void destroyNodeData(NodeData *ND) {
	freeAuxList(ND->waitingJobs);
	free(ND);
}

int checkNewStageStart(UserJobData *J, Model *M) {
	int i, j, canStart, someNewStart;

	someNewStart = 0;
	for(i = 0; i < M->nStages; i++) {
		if((J->stageState[i] == STAGE_ST_WAITING) && (M->Stages[i].nPre > 0)) {
			canStart = 0;
//printf("nPre:%d\n", M->Stages[i].nPre);
			for(j = 0; j < M->Stages[i].nPre; j++) {
//printf("Pre: #%d (%s)[%d] \n", j, M->Stages[i].Pre[j]->Id, M->Stages[i].Pre[j]->nId);
				if(J->stageState[M->Stages[i].Pre[j]->nId] == STAGE_ST_ENDED) {
					canStart++;
				}
			}
//printf("S:%d cs:%d\n", i, canStart);
			if(canStart == M->Stages[i].nPre) {
				J->stageState[i] = STAGE_ST_CAN_START;
				J->stagesToStart--;
				someNewStart = 1;
			}
		}
	}
	return someNewStart;
}

void createReadyList(UserJobData *J, Model *M) {
	int stg, tsk;
	sList *nEv;
	for(stg = 0; stg < M->nStages; stg++) {
		if(J->stageState[stg] == STAGE_ST_CAN_START) {
			J->stageState[stg] = STAGE_ST_RUNNING;
			for(tsk = 0; tsk < M->Stages[stg].Tasks; tsk++) {
				nEv = createEvent(0.0);
				Job *jobData = malloc(sizeof(Job));
				jobData->userId = J->uId;
				jobData->jobId = J->jId;
				jobData->phaseId = stg;
				jobData->taskId = tsk;
				nEv->data = jobData;
				addToAux(J->readyList, nEv, TAIL);
			}
		}
	}
}

void scheduleReadyTasksOnAvailableNodes(double T, sCalendarEvent *ce, NodeData *ND, UserJobData *J, PerfIdxs *PI, Model *M, LuaWrapper *Lw) {
	// Return immediately if no job to run is left
	if((int)getHowmanyAux(J->readyList) <= 0) return;

	// Lock the system if it was free
	if(ND->UserLocker == -1) {
		J->startTime = T;			// Locking marks the start of a job
		ND->UserLocker = J->uId;
	}
	
	// If the system is locked by another user, return immediately
	if(ND->UserLocker != J->uId) return;

	// Schedule waiting jobs on available nodes
	while(ND->NfreeNodes > 0) {
		sList *ev;
//printf("%d\n",(int)getHowmanyAux1(J->readyList));		
		if((int)getHowmanyAux(J->readyList) <= 0) break;
		if((int)getHowmanyAux(J->readyList) == 1) {
			ev = delItemAux(J->readyList, TAIL);
		} else {
			RPop(J->readyList, UNIFORM, &ev);
		}
//printf("Popped %p\n", ev);
		ND->NfreeNodes--;
		double SmpD = genSample(M->Stages[((Job *)(ev->data))->phaseId].Distr, Lw);
		ev->T = T + SmpD;
		measureAddSample(PI, MEAS_ID(1+((Job *)(ev->data))->phaseId,M_XST_AVE_SERV_TIME), SmpD);
		addEvent(ce, ev);
		// check if this is the first job of a new stage
		if(J->stageStart[((Job *)(ev->data))->phaseId] < 0.0) {
			J->stageStart[((Job *)(ev->data))->phaseId] = T;
		}
//printf("Scheduled, %d remaining\n", ND->NfreeNodes);
	}
	
	return;
}

void relseaseNode(double T, sCalendarEvent *ce, NodeData *ND, UserJobData **UJD, int uId, PerfIdxs *PI, Model *M, LuaWrapper *Lw) {
	UserJobData *J = UJD[uId];
	// Free one node
	ND->NfreeNodes++;
	// Remove locks if all job of the locker task have started
	if(((int)getHowmanyAux(J->readyList) <= 0) && (ND->UserLocker == J->uId) && (J->stagesToStart == 0)) {
		ND->UserLocker = -1;
		if((int)getHowmanyAux(ND->waitingJobs) > 0) {	// some jobs waiting
			sList *ev = delItemAux(ND->waitingJobs, HEAD);
			scheduleReadyTasksOnAvailableNodes(T, ce, ND, UJD[((Job *)(ev->data))->userId], PI, M, Lw);
//printf("Job in queue for user %d\n", ((Job *)(ev->data))->userId);
		}
	}

	scheduleReadyTasksOnAvailableNodes(T, ce, ND, J, PI, M, Lw);
}


#define MAX_VALS 128
// solves the model and sends the results to lua

void solve(Model *M, LuaWrapper *Lw) {
	double vals[MAX_VALS];
	int row, col;
	int i;

	PerfIdxs *PI = createPerfIdxs(M);
	
	RestartAllDistributions(M, Lw);
	
	// Simulation variables
	sList *ev, *nEv;
	// job structures
	UserJobData **UJD = (UserJobData **)calloc(sizeof(UserJobData *), M->Users);
	for(i = 0; i < M->Users; i++) {
		UJD[i] = createUserJobData(M);
	}
	NodeData *ND = initNodeData(M);

	// Event calendar	
	sCalendarEvent *ce;
	sParameters pars;
	pars.bucket_width = DEFAULT_BUCKET_WIDTH;
	pars.choice = LIST;
	pars.epsilon = 0.00001;
	pars.numberOfBuckets = DEFAULT_NUMBER_OF_BUCKETS;

	ce = createCE(&pars);

	// init events	
	for(i = 0; i < M->Users; i++) {
		double T = genSample(M->UThinkTimeDistr, Lw);
		measureAddSample(PI, MEAS_ID(0,M_AVE_THINK_TIME), T);
		nEv = addEvent(ce, createEvent(T));
		Job *jobData = malloc(sizeof(Job));
		jobData->userId = i;
		jobData->jobId = 0;
		jobData->phaseId = -1;
		jobData->taskId = 0;
		nEv->data = jobData;
	}

//	displayCE(ce);
	// main simulation loop
	int TotalJobEnded = 0;
	
	while(TotalJobEnded < M->maxJobs) {
		int evToFree = 1;
		popEvent(ce, &ev);
//printf("%p\n", ev);
//displayCE(ce);
		if(ev == NULL) break;
		Job *jd = (Job *)(ev->data);
		double currTime = ev->T;
//printf("%10.3f\t%d\t%d\t%d\t%d\t(%d,%d)\n", currTime, jd->userId, jd->jobId, jd->phaseId, jd->taskId, ND->NfreeNodes, ND->UserLocker);
		if(jd->phaseId == -1) {	// user want to submit a new job
			int uId = jd->userId;
			int jId = jd->jobId;
			initUserJobData(UJD[uId], currTime, uId, jId, M);
			createReadyList(UJD[uId], M);
			if((ND->NfreeNodes > 0) && (ND->UserLocker == -1)) {
				scheduleReadyTasksOnAvailableNodes(currTime, ce, ND, UJD[uId], PI, M, Lw);
			} else {
				addToAux(ND->waitingJobs, ev, TAIL);
				evToFree = 0;
			}
		} else {	// a task of a runing job ended
			int uId = jd->userId;
			int pId = jd->phaseId;
			UJD[uId]->remainingTasksXStage[pId]--;
//printf("%d\n",UJD[uId]->remainingTasksXStage[pId]);
			if(UJD[uId]->remainingTasksXStage[pId] <= 0) {
//printf("Stage %d ended\n", pId);
				UJD[uId]->stageEnd[pId] = currTime;
				measureAddSample(PI, MEAS_ID(1+pId,M_XST_START), UJD[uId]->stageStart[pId] - UJD[uId]->startTime);
				measureAddSample(PI, MEAS_ID(1+pId,M_XST_END), UJD[uId]->stageEnd[pId] - UJD[uId]->startTime);
				measureAddSample(PI, MEAS_ID(1+pId,M_XST_DUR), UJD[uId]->stageEnd[pId] - UJD[uId]->stageStart[pId]);
							
				UJD[uId]->stageState[pId] = STAGE_ST_ENDED;
				if(checkNewStageStart(UJD[uId], M)) {
//printf("Creating new ready list\n");
					createReadyList(UJD[uId], M);
				}
				if(((int)getHowmanyAux(UJD[uId]->readyList) <= 0) && (UJD[uId]->stagesToStart == 0)) {
					UJD[uId]->endTime = currTime;
					// Collect job PI					
					measureAddSample(PI, MEAS_ID(0,M_SYS_TOT),  UJD[uId]->endTime   - UJD[uId]->submissionTime);
					measureAddSample(PI, MEAS_ID(0,M_SYS_WAIT), UJD[uId]->startTime - UJD[uId]->submissionTime);
					measureAddSample(PI, MEAS_ID(0,M_SYS_JOB),  UJD[uId]->endTime   - UJD[uId]->startTime);
					
					int jId = jd->jobId;
//printf("\nJob %d of user %d ended\n\n", jId, uId);
					double Smp = genSample(M->UThinkTimeDistr, Lw);
					measureAddSample(PI, MEAS_ID(0,M_AVE_THINK_TIME), Smp);
					double T = currTime + Smp;
					nEv = addEvent(ce, createEvent(T));
					Job *jobData = malloc(sizeof(Job));
					jobData->userId = uId;
					jobData->jobId = jId+1;
					jobData->phaseId = -1;
					jobData->taskId = 0;
					nEv->data = jobData;

					TotalJobEnded++;
				}
			}
			relseaseNode(currTime, ce, ND, UJD, uId, PI, M, Lw);
		}
		if(evToFree == 1) {
			freeEvent(&ev);
		}
	}
//printf("%d\n", (int)getHowmanyAux1(ND->waitingJobs));
	
	// send results
	row = 1;
	col = 0;
	vals[col++] = 0;
	vals[col++] = M_SYS_TOT;
	col += makeConfInt(vals, col, PI, MEAS_ID(0,M_SYS_TOT), M->confIntCoeff);
	sendResult(row++, col, vals, Lw->L);
	col = 0;
	vals[col++] = 0;
	vals[col++] = M_SYS_WAIT;
	col += makeConfInt(vals, col, PI, MEAS_ID(0,M_SYS_WAIT), M->confIntCoeff);
	sendResult(row++, col, vals, Lw->L);
	col = 0;
	vals[col++] = 0;
	vals[col++] = M_SYS_JOB;
	col += makeConfInt(vals, col, PI, MEAS_ID(0,M_SYS_JOB), M->confIntCoeff);
	sendResult(row++, col, vals, Lw->L);
	col = 0;
	vals[col++] = 0;
	vals[col++] = M_AVE_THINK_TIME;
	col += makeConfInt(vals, col, PI, MEAS_ID(0,M_AVE_THINK_TIME), M->confIntCoeff);
	sendResult(row++, col, vals, Lw->L);

	for(i = 0; i < M->nStages; i++) {
		col = 0;
		vals[col++] = 1;
		vals[col++] = i+1;
		vals[col++] = M_XST_START;
		col += makeConfInt(vals, col, PI, MEAS_ID(i+1,M_XST_START), M->confIntCoeff);
		sendResult(row++, col, vals, Lw->L);
		col = 0;
		vals[col++] = 1;
		vals[col++] = i+1;
		vals[col++] = M_XST_END;
		col += makeConfInt(vals, col, PI, MEAS_ID(i+1,M_XST_END), M->confIntCoeff);
		sendResult(row++, col, vals, Lw->L);
		col = 0;
		vals[col++] = 1;
		vals[col++] = i+1;
		vals[col++] = M_XST_DUR;
		col += makeConfInt(vals, col, PI, MEAS_ID(i+1,M_XST_DUR), M->confIntCoeff);
		sendResult(row++, col, vals, Lw->L);
		col = 0;
		vals[col++] = 1;
		vals[col++] = i+1;
		vals[col++] = M_XST_AVE_SERV_TIME;
		col += makeConfInt(vals, col, PI, MEAS_ID(i+1,M_XST_AVE_SERV_TIME), M->confIntCoeff);
		sendResult(row++, col, vals, Lw->L);
	}
	
	// free resoruces
	freeCalendarEvent(ce);
	
	for(i = 0; i < M->Users; i++) {
		destroyUserJobData(UJD[i]);
	}
	free(UJD);
	
	destroyNodeData(ND);
	
	destroyPerfIdxs(PI);
}







// called by lua to compute the solution
int Analyze(LuaWrapper *Lw) {
	Model *M;

	SetRandomSeedTh(Lw->thId, Lw->totThs);
//	SetRandomSeedTh(0,1);
	
	M = createModel(Lw);
	lua_newtable(Lw->L);
	solve(M, Lw);
	destroyModel(M, Lw);
	
	return 1;
}




// main procedure
int main(int argc, char *argv[]) {
	
	printf("dagSim v.1.0\n");
	if(argc < 2) {
		printf("Usage: %s model.lua [extra_args]\n", argv[0]);
	} else {
		LuaLoadAndSolve("_solverLib.lua", "solver", "main", argc, argv, Analyze);
	}
	return 0;
}
