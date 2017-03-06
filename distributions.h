/*
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

typedef struct {	
	int Type;	// Distribution type
	int ParametersBlockSize;	// size of distribution parameters
	void *Parameters;			// pointer to the parameters block for the distribution
} Distribution;

typedef struct {
	char *DistrName;
	void (*ReadParameters)(Distribution *D, LuaWrapper *L);
	double (*Generate)(Distribution *D, LuaWrapper *L);
	void (*Destroy)(Distribution *D, LuaWrapper *L);
	void (*Restart)(Distribution *D, LuaWrapper *L);
	char *Descr;
} DistribTable;

double Rnd();
void SetSeed(int seed);
void SetRandomSeed();
void SetRandomSeedTh(int i, int n);
Distribution *readDistribution(LuaWrapper *L, char *field);
// destroy a distribution, freeing all its memory
void destroyDistribution(Distribution *D, LuaWrapper *L);
// generate a sample of the selected distribution
double genSample(Distribution *D, LuaWrapper *L);
// restarts a distribution
void restartDistribution(Distribution *D, LuaWrapper *L);
