/*
## Copyright 2017 Marco	Gribaudo <marco.gribaudo@polimi.it>
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
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "luaInterface.h"
#include "distributions.h"

typedef struct {
	Distribution *D;	// Distribution to test
	int N;	// number of samples to generate
} Model;



// read the entire model
Model *createModel(LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	Model *M;
	
	M = malloc(sizeof(Model));
	
	M->D = readDistribution(Lw, "D");
	M->N = readRealDef(L, "N", 1);
	
	return M;
}
// free the memory used for the whole model
void destroyModel(Model *M, LuaWrapper *L) {
	destroyDistribution(M->D, L);
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
	restartDistribution(M->D, Lw);
}

#define MAX_VALS 128
// solves the model and sends the results to lua

void solve(Model *M, LuaWrapper *Lw) {
	int i, row, col;
	double vals[MAX_VALS];
	
	RestartAllDistributions(M, Lw);
		
	row = 1;
	for(i = 0; i < M->N; i++) {
		col = 0;
		vals[col++] = genSample(M->D, Lw);
		sendResult(row++, col, vals, Lw->L);
	}
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
	
	if(argc < 2) {
		printf("Usage: %s model.lua [extra_args]\n", argv[0]);
	} else {
		LuaLoadAndSolve("_solverLib.lua", "solver", "main", argc, argv, Analyze);
	}
	return 0;
}
