#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "matrix/Matrix.h"
#include "luaInterface.h"
#include "distributions.h"

DistribTable distribTab[];

// read a distribution from lua
Distribution *readDistribution(LuaWrapper *Lw, char *field) {
	lua_State *L = Lw->L;	
	Distribution *D;
	char *dname;
	int i;

	if(field != NULL) {
		lua_getfield(L, -1, field);
	}
	
	dname = readString(L, "type");
	i = 0;
	
	while(strcmp(distribTab[i].DistrName, dname) != 0) {
		if(strcmp(distribTab[i].DistrName, "----") == 0) {
			printf("Error: distribution <%s> not found\n", dname);
			exit(0);
		}
		i++;
	}
	
	free(dname);
	
	lua_getfield(L, -1, "params");
	if (!lua_istable(L, -1)) {
		printf("Parameters table for distribution not correctyl specified\n");
		exit(0);
	}
	D = (Distribution *)malloc(sizeof(Distribution));
	distribTab[i].ReadParameters(D, Lw);
	lua_pop(L, 1);
	D->Type = i;

	if(field != NULL) {
		lua_pop(L, 1);
	}
	
	return D;
}

// destroy a distribution, freeing all its memory
void destroyDistribution(Distribution *D, LuaWrapper *L) {
	distribTab[D->Type].Destroy(D, L);
	free(D);
}


// generate a sample of the selected distribution
double genSample(Distribution *D, LuaWrapper *L) {
	double out;
	
	out = distribTab[D->Type].Generate(D, L);

	return out;
}

// restarts a distribution
void restartDistribution(Distribution *D, LuaWrapper *L) {
	distribTab[D->Type].Restart(D, L);
}


// Common destroy for simple distribution (just a simply allocated parameter block)
void COMMON_Destroy(Distribution *D, LuaWrapper *L) {
	free(D->Parameters);
};

// Common restart for simple distribution (No special restart action needed)
void COMMON_Restart(Distribution *D, LuaWrapper *L) {
};


// // // Helper function
#define MAX_ERLANG_SINGLE_LOG 100
double erlangRnd(int k, double lambda) {
	double R = 0.0, out;
	int i;
	
	out = 0.0;
	R = Rnd();
	for(i = 1; i < k; i++) {
		R *= Rnd();
		if(i % MAX_ERLANG_SINGLE_LOG == 0) {
			out += -log(R);
			R = 1.0;
		}
	}
	out += -log(R);
	out = out / lambda;
	
	// Old version
//	while(R == 0.0) {
//		R = Rnd();
//		for(i = 1; i < k; i++) {
//			R *= Rnd();
//		}
//	}
//	out = -log(R) / lambda;
	if(!isfinite(out)) {
		printf("%d %g %g %g ERROR!!!!\n", k, lambda, R, out);
		exit(0);
	}
	return out;
}

double BoxMullerNorm(double *val) {
	if(val[3] > 0.0) {
		val[3] = -1.0;
		return val[2];
	} else {
		double r = sqrt(-2.0*log(Rnd())) * val[1];
		double a = 2.0 * M_PI * Rnd();
		val[2] = cos(a) * r + val[0];
		val[3] = 1.0;
		return (sin(a) * r + val[0]);
	}
}



/***** Hyper Sum of Erlang distributions ****/

typedef struct {
	int Ns;
	double prob;

	double *lambda;
	int *k;
} HSE_block;

typedef struct {
	int Nb;
	HSE_block *Hbs;
} HSE_params;

void HSE_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	int i, j;
	HSE_params *HSE;
	D->ParametersBlockSize = sizeof(HSE_params);
	HSE = (HSE_params *)malloc(sizeof(HSE_params));
	D->Parameters = HSE;
	
//printf("Startup (%d)\n", lua_gettop(L));	
	HSE->Nb = countElementsTopTable(L);
	HSE->Hbs = (HSE_block *)calloc(HSE->Nb, sizeof(HSE_block));
//printf("Found %d blocks (%d)\n", HSE->Nb, lua_gettop(L));	
	for(i = 0; i < HSE->Nb; i++) {
		lua_pushinteger(L, i+1);
		lua_gettable(L, -2);
//printf("Reading block %d (%d)\n", i+1, lua_gettop(L));	
		
		HSE->Hbs[i].Ns = countElementsTopTable(L)-1;
//printf("Found %d stages (%d)\n", HSE->Hbs[i].Ns, lua_gettop(L));	
		if(HSE->Hbs[i].Ns <= 0) {
			printf("Error in defining HSE distribution\n");
			exit(0);
		}
		
		// Read the probability
		lua_pushinteger(L, 1);
		lua_gettable(L, -2);
		HSE->Hbs[i].prob = lua_tonumber(L, -1);
		lua_pop(L, 1);
//printf("Prob %g (%d)\n", HSE->Hbs[i].prob, lua_gettop(L));	
		
		// Allocate stages
		HSE->Hbs[i].k = (int *)calloc(HSE->Hbs[i].Ns, sizeof(int));
		HSE->Hbs[i].lambda = (double *)calloc(HSE->Hbs[i].Ns, sizeof(double));
		
		// Read stages
		for(j = 0; j < HSE->Hbs[i].Ns; j++) {
//printf("Reading stage %d (%d)\n", j+1, lua_gettop(L));	
			lua_pushinteger(L, j+2);
			lua_gettable(L, -2);
			
			HSE->Hbs[i].k[j] = readRealDef(L, "k", 1);
			HSE->Hbs[i].lambda[j] = readReal(L, "lambda");
//printf("K %d, lambda %g (%d)\n", HSE->Hbs[i].k[j], HSE->Hbs[i].lambda[j], lua_gettop(L));	
			
			lua_pop(L, 1);
		}
		
		lua_pop(L, 1);
	}
//	lua_pop(L, 1);
//printf("Shutdown (%d)\n", lua_gettop(L));	
}

double HSE_Generate(Distribution *D, LuaWrapper *L) {
	double out;
	int i, j;
	double r;
	HSE_params *HSE = (HSE_params *)D->Parameters;

	out = 0.0;
	
	// get the block
	r = Rnd();
	i = 0;
	while((r > HSE->Hbs[i].prob) && (i < HSE->Nb)) {
		r -=  HSE->Hbs[i].prob;
		i++;
	}
	HSE_block *Hbs = &(HSE->Hbs[i]);
	
	j = 0;
	out = erlangRnd(Hbs->k[j], Hbs->lambda[j]);
	for(j = 1; j < Hbs->Ns; j++) {
		out += erlangRnd(Hbs->k[j], Hbs->lambda[j]);
	}
if(!isfinite(out)) {
	printf("%d %d %d %g  ERROR!!!!\n", i, HSE->Nb, Hbs->Ns, out);
	exit(0);
}
//printf("%d %d %d %g\n", i, HSE->Nb, Hbs->Ns, out);
	return out;
}

void HSE_Destroy(Distribution *D, LuaWrapper *L) {
	int i;
	HSE_params *HSE = (HSE_params *)D->Parameters;

	for(i = 0; i < HSE->Nb; i++) {
		free(HSE->Hbs[i].k);
		free(HSE->Hbs[i].lambda);
	}
	free(HSE->Hbs);
	free(HSE);
};






/***** PHASE TYPE and DISCRETE PHASE TYPE ****/

typedef struct {
	// read parameters
	int N;
	Vector *alpha;
	Matrix *Q;
} PH_params;

void PH_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	int i, j;
	PH_params *PH;
	D->ParametersBlockSize = sizeof(PH_params);
	PH = (PH_params *)malloc(sizeof(PH_params));
	D->Parameters = PH;
	
	PH->N = countElements(L, "alpha");
	PH->alpha = CreateVectorEnc(PH->N, VE_DIRECT);
	PH->Q = CreateMatrixEnc(PH->N, PH->N, ME_DIRECT);
	
	lua_getfield(L, -1, "alpha");
	for(i = 0; i < PH->N; i++) {
		lua_pushinteger(L, i+1);
		lua_gettable(L, -2);
		WriteVec(PH->alpha, i, lua_tonumber(L, -1));
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	lua_getfield(L, -1, "Q");
	for(i = 0; i < PH->N; i++) {
		lua_pushinteger(L, i+1);
		lua_gettable(L, -2);
		for(j = 0; j < PH->N; j++) {
			lua_pushinteger(L, j+1);
			lua_gettable(L, -2);
			WriteMat(PH->Q, i, j, lua_tonumber(L, -1));
			lua_pop(L, 1);
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}

#define MAX_ITER_PH 10000

double PH_Generate(Distribution *D, LuaWrapper *L) {
	double out;
	int St, i, j;
	double r;
	PH_params *PH = (PH_params *)D->Parameters;

	out = 0.0;
	
	// get the initial state
	St = 0;
	r = Rnd();
	while(r > ReadVec(PH->alpha, St)) {
		r -=  ReadVec(PH->alpha, St);
		St ++;
	}

	if(St < PH->N) {
		for(i = 0; i < MAX_ITER_PH; i++) {
			r = - Rnd() * ReadMat(PH->Q, St, St);
			out += log(Rnd()) / ReadMat(PH->Q, St, St);
			for(j = 0; j < PH->N; j++) {
				if(j != St) {
					if(r > ReadMat(PH->Q, St, j)) {
						r -= ReadMat(PH->Q, St, j);
					} else {
						St = j;
						break;
					}
				}
			}
			if(j == PH->N) {
				break;
			}
		}
		if(i == MAX_ITER_PH) {
			printf("\n\n\n ERROR! Phase type could not converge: maybe there are no exits?\n\n\n");
		}
	}
	return out;
}

void PH_Destroy(Distribution *D, LuaWrapper *L) {
	PH_params *PH = (PH_params *)D->Parameters;
	DestroyVector(PH->alpha);
	DestroyMatrix(PH->Q);
	free(D->Parameters);
};

// discrete case
double DPH_Generate(Distribution *D, LuaWrapper *L) {
	double out;
	int St, i, j;
	double r;
	PH_params *PH = (PH_params *)D->Parameters;

	out = 0.0;
	
	// get the initial state
	St = 0;
	r = Rnd();
	while(r > ReadVec(PH->alpha, St)) {
		r -=  ReadVec(PH->alpha, St);
		St ++;
	}

	if(St < PH->N) {
		for(i = 0; i < MAX_ITER_PH; i++) {
			r = Rnd();
			out ++;
			for(j = 0; j < PH->N; j++) {
				if(r > ReadMat(PH->Q, St, j)) {
					r -= ReadMat(PH->Q, St, j);
				} else {
					St = j;
					break;
				}
			}
			if(j == PH->N) {
				break;
			}
		}
		if(i == MAX_ITER_PH) {
			printf("\n\n\n ERROR! Discrete phase type could not converge: maybe there are no exits?\n\n\n");
		}
	}
	return out;
}



/***** MAP and DMAP TYPEs ****/
typedef struct {
	// read parameters
	int N;
	Vector *P0;	// initial state
	Matrix *D0;	// local transitions
	Matrix *D1;	// arrival transitions
	// execution parameters
	int curSt;
} MAP_params;


void MAP_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;

	int i, j;
	MAP_params *MAP;
	D->ParametersBlockSize = sizeof(MAP_params);
	MAP = (MAP_params *)malloc(sizeof(MAP_params));
	D->Parameters = MAP;
	
	MAP->N = countElements(L, "P0");
	MAP->P0 = CreateVectorEnc(MAP->N, VE_DIRECT);
	MAP->D0 = CreateMatrixEnc(MAP->N, MAP->N, ME_DIRECT);
	MAP->D1 = CreateMatrixEnc(MAP->N, MAP->N, ME_DIRECT);
	
	// reads the initial state
	lua_getfield(L, -1, "P0");
	for(i = 0; i < MAP->N; i++) {
		lua_pushinteger(L, i+1);
		lua_gettable(L, -2);
		WriteVec(MAP->P0, i, lua_tonumber(L, -1));
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	// reads the local transition matrix
	lua_getfield(L, -1, "D0");
	for(i = 0; i < MAP->N; i++) {
		lua_pushinteger(L, i+1);
		lua_gettable(L, -2);
		for(j = 0; j < MAP->N; j++) {
			lua_pushinteger(L, j+1);
			lua_gettable(L, -2);
			WriteMat(MAP->D0, i, j, lua_tonumber(L, -1));
			lua_pop(L, 1);
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	// reads the generation transition matrix
	lua_getfield(L, -1, "D1");
	for(i = 0; i < MAP->N; i++) {
		lua_pushinteger(L, i+1);
		lua_gettable(L, -2);
		for(j = 0; j < MAP->N; j++) {
			lua_pushinteger(L, j+1);
			lua_gettable(L, -2);
			WriteMat(MAP->D1, i, j, lua_tonumber(L, -1));
			lua_pop(L, 1);
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}

#define MAX_ITER_MAP 10000

double MAP_Generate(Distribution *D, LuaWrapper *L) {
	double out;
	int St, i, j;
	double r;
	MAP_params *MAP = (MAP_params *)D->Parameters;
	St = MAP->curSt;

	out = 0.0;
	
	// get the initial state
	St = 0;
	r = Rnd();
	while(r > ReadVec(MAP->P0, St)) {
		r -=  ReadVec(MAP->P0, St);
		St ++;
	}

	// Generate
	if(St < MAP->N) {
		for(i = 0; i < MAX_ITER_MAP; i++) {
			r = - Rnd() * ReadMat(MAP->D0, St, St);
			out += log(Rnd()) / ReadMat(MAP->D0, St, St);
	
			// checks for the local transitions
			for(j = 0; j < MAP->N; j++) {
				if(j != St) {
					if(r > ReadMat(MAP->D0, St, j)) {
						r -= ReadMat(MAP->D0, St, j);
					} else {
						St = j;
						break;
					}
				}
			}
			if(j == MAP->N) {
				// checks generation transitions
				for(j = 0; j < MAP->N; j++) {
					if(r > ReadMat(MAP->D1, St, j)) {
						r -= ReadMat(MAP->D1, St, j);
					} else {
						St = j;
						break;
					}
				}
				// an arrival has been generated
				break;	// exit the loop!
			}
		}
		if(i == MAX_ITER_MAP) {
			printf("\n\n\n ERROR! Phase type could not converge: maybe there are no exits?\n\n\n");
		}
	}	
	// stores the current MAP state for the next sample
	MAP->curSt = St;
	return out;
}

void MAP_Destroy(Distribution *D, LuaWrapper *L) {
	MAP_params *MAP = (MAP_params *)D->Parameters;
	DestroyVector(MAP->P0);
	DestroyMatrix(MAP->D0);
	DestroyMatrix(MAP->D1);
	free(D->Parameters);
};

void MAP_Restart(Distribution *D, LuaWrapper *L) {
	int St;
	double r;
	MAP_params *MAP = (MAP_params *)D->Parameters;
	St = MAP->curSt;

	// get the initial state
	St = 0;
	r = Rnd();
	while(r > ReadVec(MAP->P0, St)) {
		r -=  ReadVec(MAP->P0, St);
		St ++;
	}
	MAP->curSt = St;
}

// discrete case
double DMAP_Generate(Distribution *D, LuaWrapper *L) {
	double out;
	int St, i, j;
	double r;
	MAP_params *MAP = (MAP_params *)D->Parameters;
	St = MAP->curSt;

	out = 0.0;
	
	// get the initial state
	St = 0;
	r = Rnd();
	while(r > ReadVec(MAP->P0, St)) {
		r -=  ReadVec(MAP->P0, St);
		St ++;
	}

	// Generate
	if(St < MAP->N) {
		for(i = 0; i < MAX_ITER_MAP; i++) {
			r = Rnd();
			out ++;
	
			// checks for the local transitions
			for(j = 0; j < MAP->N; j++) {
				if(r > ReadMat(MAP->D0, St, j)) {
					r -= ReadMat(MAP->D0, St, j);
				} else {
					St = j;
					break;
				}
			}
			if(j == MAP->N) {
				// checks generation transitions
				for(j = 0; j < MAP->N; j++) {
					if(r > ReadMat(MAP->D1, St, j)) {
						r -= ReadMat(MAP->D1, St, j);
					} else {
						St = j;
						break;
					}
				}
				// an arrival has been generated
				break;	// exit the loop!
			}
		}
		if(i == MAX_ITER_MAP) {
			printf("\n\n\n ERROR! Phase type could not converge: maybe there are no exits?\n\n\n");
		}
	}	
	// stores the current MAP state for the next sample
	MAP->curSt = St;
	return out;
}


/***** ERLANG ****/

void erl_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	double *val;
	D->ParametersBlockSize = 2*sizeof(double);
	D->Parameters = malloc(2*sizeof(double));
	val = (double *)D->Parameters;

	val[0] = readReal(L, "k");
	val[1] = readReal(L, "lambda");
}

double erl_Generate(Distribution *D, LuaWrapper *L) {
	double *val = (double *)D->Parameters;
	return erlangRnd((int)val[0], val[1]);
}


/***** EXPONENTIAL ****/

void exp_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	double *rate;
	D->ParametersBlockSize = sizeof(double);
	D->Parameters = malloc(sizeof(double));
	rate = (double *)D->Parameters;

	*rate = readReal(L, "rate");
}

double exp_Generate(Distribution *D, LuaWrapper *L) {
	double *rate = (double *)D->Parameters;
	return -log(Rnd()) / (*rate);
}


/***** DETERMINISTIC ****/

void det_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	double *val;
	D->ParametersBlockSize = sizeof(double);
	D->Parameters = malloc(sizeof(double));
	val = (double *)D->Parameters;

	*val = readReal(L, "val");
}

double det_Generate(Distribution *D, LuaWrapper *L) {
	double *val = (double *)D->Parameters;
	return (*val);
}


/***** UNIFORM ****/

void unif_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	double *val;
	D->ParametersBlockSize = 2*sizeof(double);
	D->Parameters = malloc(2*sizeof(double));
	val = (double *)D->Parameters;

	val[0] = readReal(L, "min");
	val[1] = readReal(L, "max");
}

double unif_Generate(Distribution *D, LuaWrapper *L) {
	double *val = (double *)D->Parameters;
	double R = Rnd();
	return (val[0] * R + val[1] * (1.0 - R));
}

/***** NORMAL ****/

void norm_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	double *val;
	D->ParametersBlockSize = 4*sizeof(double);
	D->Parameters = malloc(4*sizeof(double));
	val = (double *)D->Parameters;

	val[0] = readReal(L, "mu");
	val[1] = readReal(L, "sigma");
	val[2] = 0.0;		// the previous sample generated by the Box Muller
	val[3] = -1.0;		// > 0 if the previous sample is valid, < 0 if a new couple must be generated
}

double norm_Generate(Distribution *D, LuaWrapper *L) {
	double *val = (double *)D->Parameters;

	return BoxMullerNorm(val);
}

void norm_Restart(Distribution *D, LuaWrapper *L) {
	double *val = (double *)D->Parameters;
	val[3] = -1.0;
}

/***** Positive NORMAL ****/

void Pnorm_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	double *val;
	char *tech;
	
	D->ParametersBlockSize = 5*sizeof(double);
	D->Parameters = malloc(5*sizeof(double));
	val = (double *)D->Parameters;

	val[0] = readReal(L, "mu");
	val[1] = readReal(L, "sigma");
	val[2] = 0.0;		// the previous sample generated by the Box Muller
	val[3] = -1.0;		// > 0 if the previous sample is valid, < 0 if a new couple must be generated

	tech = readStringDef(L, "tech", "trunc");
	if(strcmp("trunc", tech) == 0) {
		val[4] = 0.0;
	} else if(strcmp("abs", tech) == 0) {
		val[4] = 1.0;
	} else if(strcmp("refl", tech) == 0) {
		val[4] = 2.0;
	} else {
		printf("Unknown technique for positive normal: %s\n", tech);
		exit(0);
	}

	free(tech);
	
}

#define P_NORM_TRUNC_MAX_RETRY 10000

double Pnorm_Generate(Distribution *D, LuaWrapper *L) {
	double *val = (double *)D->Parameters;
	double v = BoxMullerNorm(val);
	int i;
	
	switch((int)val[4]) {
		case 0:
			for(i = 0; i < P_NORM_TRUNC_MAX_RETRY; i++) {
				v = BoxMullerNorm(val);
				if(v >= 0) break;
			}
			break;
		case 1:
			if(v < 0.0) v = 0.0;
			break;
		case 2:
			if(v < 0.0) v = -v;
			break;
		default:
			printf("Unknown technique for positive normal: %g [This cannot happened]\n", val[4]);
			exit(0);
			break;
	}

	return v;
}

/***** LOG-NORMAL ****/

void lognorm_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	double *val;
	D->ParametersBlockSize = 4*sizeof(double);
	D->Parameters = malloc(4*sizeof(double));
	val = (double *)D->Parameters;

	val[0] = readReal(L, "mu");
	val[1] = readReal(L, "sigma");
	val[2] = 0.0;		// the previous sample generated by the Box Muller
	val[3] = -1.0;		// > 0 if the previous sample is valid, < 0 if a new couple must be generated
}

double lognorm_Generate(Distribution *D, LuaWrapper *L) {
	double *val = (double *)D->Parameters;

	return exp(BoxMullerNorm(val));
}

/***** PARETO ****/

void pareto_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	double *val;
	D->ParametersBlockSize = 2*sizeof(double);
	D->Parameters = malloc(2*sizeof(double));
	val = (double *)D->Parameters;

	val[0] = readReal(L, "xm");
	val[1] = 1.0 / readReal(L, "alpha");
}

double pareto_Generate(Distribution *D, LuaWrapper *L) {
	double *val = (double *)D->Parameters;
	return val[0] / pow(Rnd(), val[1]);
}

/***** WEIBULL ****/

void weib_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	double *val;
	D->ParametersBlockSize = 2*sizeof(double);
	D->Parameters = malloc(2*sizeof(double));
	val = (double *)D->Parameters;

	val[0] = readReal(L, "k");
	val[1] = readReal(L, "lambda");
}

double weib_Generate(Distribution *D, LuaWrapper *L) {
	double *val = (double *)D->Parameters;
	return val[1] * pow(-log(Rnd()), 1.0 / val[0]);
}

/***** LOGISTIC ****/

void logis_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	double *val;
	D->ParametersBlockSize = 2*sizeof(double);
	D->Parameters = malloc(2*sizeof(double));
	val = (double *)D->Parameters;

	val[0] = readReal(L, "mu");
	val[1] = readReal(L, "s");
}

double logis_Generate(Distribution *D, LuaWrapper *L) {
	double *val = (double *)D->Parameters;
	double mu = val[0], s = val[1], p = Rnd();
	return mu + s * log(p / (1-p));
}

/***** LOG-LOGISTIC ****/

void loglogis_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	double *val;
	D->ParametersBlockSize = 2*sizeof(double);
	D->Parameters = malloc(2*sizeof(double));
	val = (double *)D->Parameters;

	val[0] = readReal(L, "alpha");
	val[1] = 1.0 / readReal(L, "beta");
}

double loglogis_Generate(Distribution *D, LuaWrapper *L) {
	double *val = (double *)D->Parameters;
	double alpha = val[0], oneOverBeta = val[1], p = Rnd();
	return alpha * pow(p / (1-p), oneOverBeta);
}

/***** EMPIRICAL DISTRIBUTION ****/

typedef struct {
	// read parameters
	int N;
	Vector *S;
} Emp_params;

void emp_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	int i;
	Emp_params *E;
	D->ParametersBlockSize = sizeof(Emp_params);
	E = (Emp_params *)malloc(sizeof(Emp_params));
	D->Parameters = E;
	
	E->N = countElements(L, "samples");
	E->S = CreateVectorEnc(E->N, VE_DIRECT);
	
	lua_getfield(L, -1, "samples");
	for(i = 0; i < E->N; i++) {
		lua_pushinteger(L, i+1);
		lua_gettable(L, -2);
		WriteVec(E->S, i, lua_tonumber(L, -1));
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}

double emp_Generate(Distribution *D, LuaWrapper *L) {
	Emp_params *E = (Emp_params *)D->Parameters;

	return ReadVec(E->S, (int)floor(Rnd() * (double)E->N));
}

void emp_Destroy(Distribution *D, LuaWrapper *L) {
	Emp_params *E = (Emp_params *)D->Parameters;
	DestroyVector(E->S);
	free(D->Parameters);
};



/***** REPLAY DISTRIBUTION ****/

typedef struct {
	// read parameters
	int N;
	int curr;
	Vector *S;
} Replay_params;

void replay_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	int i;
	Replay_params *E;
	D->ParametersBlockSize = sizeof(Replay_params);
	E = (Replay_params *)malloc(sizeof(Replay_params));
	D->Parameters = E;
	
	E->N = countElements(L, "samples");
	E->S = CreateVectorEnc(E->N, VE_DIRECT);
	E->curr = 0;
	
	lua_getfield(L, -1, "samples");
	for(i = 0; i < E->N; i++) {
		lua_pushinteger(L, i+1);
		lua_gettable(L, -2);
		WriteVec(E->S, i, lua_tonumber(L, -1));
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}

double replay_Generate(Distribution *D, LuaWrapper *L) {
	Replay_params *E = (Replay_params *)D->Parameters;
	
	double out = ReadVec(E->S, E->curr);
	E->curr = (E->curr + 1) % E->N;
	
	return out;
}

void replay_Destroy(Distribution *D, LuaWrapper *L) {
	Replay_params *E = (Replay_params *)D->Parameters;
	DestroyVector(E->S);
	free(D->Parameters);
};


void replay_Restart(Distribution *D, LuaWrapper *L) {
	Replay_params *E = (Replay_params *)D->Parameters;
	E->curr = 0;
}





/***** HISTOGRAM ****/

typedef struct {
	// read parameters
	int N;
	Vector *w;
	double size;
	double min;
	double wTot;
} Histo_params;

void histo_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	int i;
	Histo_params *HP;
	D->ParametersBlockSize = sizeof(Histo_params);
	HP = (Histo_params *)malloc(sizeof(Histo_params));
	D->Parameters = HP;
	
	HP->N = countElements(L, "w");
	HP->w = CreateVectorEnc(HP->N, VE_DIRECT);
	HP->wTot = 0.0;
	HP->size = readReal(L, "size");
	HP->min = readRealDef(L, "min", 0);	
	
	lua_getfield(L, -1, "w");
	for(i = 0; i < HP->N; i++) {
		double v;
		lua_pushinteger(L, i+1);
		lua_gettable(L, -2);
		v = lua_tonumber(L, -1);
		WriteVec(HP->w, i, v);
		HP->wTot += v;
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}

double histo_Generate(Distribution *D, LuaWrapper *L) {
	int St;
	double r;
	Histo_params *HP = (Histo_params *)D->Parameters;

	// get the initial state
	St = 0;
	r = Rnd() * HP->wTot;
	while(r > ReadVec(HP->w, St)) {
		r -=  ReadVec(HP->w, St);
		St ++;
	}
	return ((double)St + HP->min + Rnd()) * HP->size;
}

void histo_Destroy(Distribution *D, LuaWrapper *L) {
	Histo_params *HP = (Histo_params *)D->Parameters;
	DestroyVector(HP->w);
	free(D->Parameters);
};








/***** BERNOULLI ****/

void bern_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	double *val;
	D->ParametersBlockSize = sizeof(double);
	D->Parameters = malloc(sizeof(double));
	val = (double *)D->Parameters;

	*val = readReal(L, "p");
}

double bern_Generate(Distribution *D, LuaWrapper *L) {
	double *val = (double *)D->Parameters;
	return (Rnd() < (*val) ? 0.0 : 1.0);
}


/***** DISCRETE UNIFORM ****/

void dunif_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	double *val;
	D->ParametersBlockSize = 2*sizeof(double);
	D->Parameters = malloc(2*sizeof(double));
	val = (double *)D->Parameters;

	val[0] = readReal(L, "min");
	val[1] = readReal(L, "max");
}

double dunif_Generate(Distribution *D, LuaWrapper *L) {
	double *val = (double *)D->Parameters;
	double R = Rnd();
	return val[0] + floor(R * (val[1] - val[0] + 1));
}


/***** GEOMETRIC ****/

void geom_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	double *val;
	D->ParametersBlockSize = sizeof(double);
	D->Parameters = malloc(sizeof(double));
	val = (double *)D->Parameters;

	*val = readReal(L, "p");
}

double geom_Generate(Distribution *D, LuaWrapper *L) {
	double *p = (double *)D->Parameters;
	return floor(log(Rnd()) / log(1.0 - (*p)));;
}

/***** POISSON ****/

void pois_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	double *lambda;
	D->ParametersBlockSize = sizeof(double);
	D->Parameters = malloc(sizeof(double));
	lambda = (double *)D->Parameters;

	*lambda = readReal(L, "lambda");
}

#define POIS_STEP 100.0
#define E_POIS_STEP 2.6881171418161356e+43

double pois_Generate(Distribution *D, LuaWrapper *L) {
	double *lambda = (double *)D->Parameters;
	double lambda_left = (*lambda), p = 1.0;
	int k = 0;
	do {
		k++;
		p *= Rnd();
		if((p < M_E) && (lambda_left > 0.0)) {
			if(lambda_left > POIS_STEP) {
				p *= E_POIS_STEP;
//				lambda_left -= POIS_STEP;
			} else {
				p *= exp(lambda_left);
				lambda_left = -1;
			}
		}
	} while(p > 1.0);
	return k - 1;
}


/***** DISCRETE ****/

typedef struct {
	// read parameters
	int N;
	Vector *w;
	double wTot;
} Disc_params;

void disc_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	int i;
	Disc_params *DP;
	D->ParametersBlockSize = sizeof(Disc_params);
	DP = (Disc_params *)malloc(sizeof(Disc_params));
	D->Parameters = DP;
	
	DP->N = countElements(L, "w");
	DP->w = CreateVectorEnc(DP->N, VE_DIRECT);
	DP->wTot = 0.0;
	
	lua_getfield(L, -1, "w");
	for(i = 0; i < DP->N; i++) {
		double v;
		lua_pushinteger(L, i+1);
		lua_gettable(L, -2);
		v = lua_tonumber(L, -1);
		WriteVec(DP->w, i, v);
		DP->wTot += v;
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}

double disc_Generate(Distribution *D, LuaWrapper *L) {
	int St;
	double r;
	Disc_params *DP = (Disc_params *)D->Parameters;

	// get the initial state
	St = 0;
	r = Rnd() * DP->wTot;
	while(r > ReadVec(DP->w, St)) {
		r -=  ReadVec(DP->w, St);
		St ++;
	}
	return St;
}

void disc_Destroy(Distribution *D, LuaWrapper *L) {
	Disc_params *DP = (Disc_params *)D->Parameters;
	DestroyVector(DP->w);
	free(D->Parameters);
};


/***** DISCRETE FROM A SET ****/

typedef struct {
	// read parameters
	int N;
	Vector *w;
	Vector *v;
	double wTot;
} Dset_params;

void dset_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	int i;
	Dset_params *DP;
	D->ParametersBlockSize = sizeof(Dset_params);
	DP = (Dset_params *)malloc(sizeof(Dset_params));
	D->Parameters = DP;
	
	DP->N = countElements(L, "w");
	DP->w = CreateVectorEnc(DP->N, VE_DIRECT);
	DP->v = CreateVectorEnc(DP->N, VE_DIRECT);
	DP->wTot = 0.0;
	
	lua_getfield(L, -1, "w");
	for(i = 0; i < DP->N; i++) {
		double v;
		lua_pushinteger(L, i+1);
		lua_gettable(L, -2);
		v = lua_tonumber(L, -1);
		WriteVec(DP->w, i, v);
		DP->wTot += v;
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	lua_getfield(L, -1, "v");
	for(i = 0; i < DP->N; i++) {
		lua_pushinteger(L, i+1);
		lua_gettable(L, -2);
		WriteVec(DP->v, i, lua_tonumber(L, -1));
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}

double dset_Generate(Distribution *D, LuaWrapper *L) {
	int St;
	double r;
	Dset_params *DP = (Dset_params *)D->Parameters;

	// get the initial state
	St = 0;
	r = Rnd() * DP->wTot;
	while(r > ReadVec(DP->w, St)) {
		r -=  ReadVec(DP->w, St);
		St ++;
	}
	return ReadVec(DP->v, St);
}

void dset_Destroy(Distribution *D, LuaWrapper *L) {
	Dset_params *DP = (Dset_params *)D->Parameters;
	DestroyVector(DP->w);
	DestroyVector(DP->v);
	free(D->Parameters);
};

/***** Choice or Linear combination common procedures and definitions ****/

typedef struct {
	double w;
	Distribution *D;
} ChoiceLin_block;

typedef struct {
	int Nb;
	ChoiceLin_block *CLB;
	
	double wTotOrConst;
} ChoiceLin_params;

void choiceLin_Destroy(Distribution *D, LuaWrapper *L) {
	int i;
	ChoiceLin_params *CLP = (ChoiceLin_params *)D->Parameters;

	for(i = 0; i < CLP->Nb; i++) {
		if(CLP->CLB[i].D != NULL) {
			destroyDistribution(CLP->CLB[i].D, L);
		}
	}
	free(CLP->CLB);
	free(CLP);
};

void choiceLin_Restart(Distribution *D, LuaWrapper *L) {
	int i;
	ChoiceLin_params *CLP = (ChoiceLin_params *)D->Parameters;

	for(i = 0; i < CLP->Nb; i++) {
		if(CLP->CLB[i].D != NULL) {
			restartDistribution(CLP->CLB[i].D, L);
		}
	}
}

/***** Linear combinations of a Random Variables ****/

void lin_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	int i;
	ChoiceLin_params *CLP;
	D->ParametersBlockSize = sizeof(ChoiceLin_params);
	CLP = (ChoiceLin_params *)malloc(sizeof(ChoiceLin_params));
	D->Parameters = CLP;
	
	CLP->Nb = countElementsTopTable(L);
	CLP->CLB = (ChoiceLin_block *)calloc(CLP->Nb, sizeof(ChoiceLin_block));
	CLP->wTotOrConst = 0.0;
	
	for(i = 0; i < CLP->Nb; i++) {
		lua_pushinteger(L, i+1);
		lua_gettable(L, -2);
		
		switch(countElementsTopTable(L)) {
		  case 1:
			// Read the constantTerm
			lua_pushinteger(L, 1);
			lua_gettable(L, -2);
			CLP->CLB[i].w = lua_tonumber(L, -1);
			CLP->CLB[i].D = NULL;
			CLP->wTotOrConst += CLP->CLB[i].w;
			lua_pop(L, 1);
		    break;
		    
		  case 2:
			// Read the coefficient
			lua_pushinteger(L, 1);
			lua_gettable(L, -2);
			CLP->CLB[i].w = lua_tonumber(L, -1);
			lua_pop(L, 1);
			
			// Read the distribution
			lua_pushinteger(L, 2);
			lua_gettable(L, -2);
			CLP->CLB[i].D = readDistribution(Lw, NULL);
			lua_pop(L, 1);
		    break;
		    
		  default:
			printf("Error in defining Lin distribution\n");
			exit(0);
			break;
		}
		
		
		lua_pop(L, 1);
	}
//	lua_pop(L, 1);
}


double lin_Generate(Distribution *D, LuaWrapper *L) {
	int i;
	double out;
	ChoiceLin_params *CLP = (ChoiceLin_params *)D->Parameters;
	out = CLP->wTotOrConst;
	for(i = 0; i < CLP->Nb; i++) {
		if(CLP->CLB[i].D != NULL) {
			out += genSample(CLP->CLB[i].D, L) * CLP->CLB[i].w;	
		}
	}
	return out;
}


/***** Probabilistic choice of a Random Variable ****/

void choice_ReadParameters(Distribution *D, LuaWrapper *Lw) {
	lua_State *L = Lw->L;
	int i;
	ChoiceLin_params *CLP;
	D->ParametersBlockSize = sizeof(ChoiceLin_params);
	CLP = (ChoiceLin_params *)malloc(sizeof(ChoiceLin_params));
	D->Parameters = CLP;
	
	CLP->Nb = countElementsTopTable(L);
	CLP->CLB = (ChoiceLin_block *)calloc(CLP->Nb, sizeof(ChoiceLin_block));
	CLP->wTotOrConst = 0.0;
//printf("Found %d blocks (%d)\n", HSE->Nb, lua_gettop(L));	
	for(i = 0; i < CLP->Nb; i++) {
		lua_pushinteger(L, i+1);
		lua_gettable(L, -2);
//printf("Reading block %d (%d)\n", i+1, lua_gettop(L));	
		
		if(countElementsTopTable(L) != 2) {
			printf("Error in defining Choice distribution\n");
			exit(0);
		}
		
		// Read the probability
		lua_pushinteger(L, 1);
		lua_gettable(L, -2);
		CLP->CLB[i].w = lua_tonumber(L, -1);
		CLP->wTotOrConst += CLP->CLB[i].w;
		lua_pop(L, 1);
//printf("Prob %g (%d)\n", HSE->Hbs[i].prob, lua_gettop(L));	
		
		// Read the distribution
		lua_pushinteger(L, 2);
		lua_gettable(L, -2);
		CLP->CLB[i].D = readDistribution(Lw, NULL);
		lua_pop(L, 1);
		
		lua_pop(L, 1);
	}
//	lua_pop(L, 1);
//printf("Shutdown (%d)\n", lua_gettop(L));	
}

double choice_Generate(Distribution *D, LuaWrapper *L) {
	double out;
	int i;
	double r;
	ChoiceLin_params *CLP = (ChoiceLin_params *)D->Parameters;

	out = 0.0;
	
	// get the block
	r = Rnd() * CLP->wTotOrConst;
	i = 0;
	while((r > CLP->CLB[i].w) && (i < CLP->Nb)) {
		r -=  CLP->CLB[i].w;
		i++;
	}

	out = genSample(CLP->CLB[i].D, L);	
	return out;
}




DistribTable distribTab[] = {
	// Based on expansions
	{"PH",			PH_ReadParameters,		PH_Generate,		PH_Destroy,			COMMON_Restart,
	 "Phase type"},
	{"MAP",			MAP_ReadParameters,		MAP_Generate,		MAP_Destroy,		MAP_Restart,
	 "Markov arrival process"},
	{"HSE",			HSE_ReadParameters,		HSE_Generate,		HSE_Destroy,		COMMON_Restart,
	 "Hyper sum of Erlangs"},
	{"DPH",			PH_ReadParameters,		DPH_Generate,		PH_Destroy,			COMMON_Restart,
	 "Discrete phase type"},
	{"DMAP",		MAP_ReadParameters,		DMAP_Generate,		MAP_Destroy,		MAP_Restart,
	 "Discrete Markov arrival process"},

	// Continuous
	{"erl",			erl_ReadParameters,		erl_Generate,		COMMON_Destroy,		COMMON_Restart,
	 "Erlang"},
	{"exp",			exp_ReadParameters,		exp_Generate,		COMMON_Destroy,		COMMON_Restart,
	 "Exponential"},
	{"det",			det_ReadParameters,		det_Generate,		COMMON_Destroy,		COMMON_Restart,
	 "Deterministic"},
	{"unif",		unif_ReadParameters,	unif_Generate,		COMMON_Destroy,		COMMON_Restart,
	 "Uniform"},
	{"norm",		norm_ReadParameters,	norm_Generate,		COMMON_Destroy,		norm_Restart,
	 "Normal"},
	{"Pnorm",		Pnorm_ReadParameters,	Pnorm_Generate,		COMMON_Destroy,		norm_Restart,
	 "Positive normal"},
	{"lognorm",		lognorm_ReadParameters,	lognorm_Generate,	COMMON_Destroy,		norm_Restart,
	 "Log-normal"},
	{"pareto",		pareto_ReadParameters,	pareto_Generate,	COMMON_Destroy,		COMMON_Restart,
	 "Pareto"},
	{"weib",		weib_ReadParameters,	weib_Generate,		COMMON_Destroy,		COMMON_Restart,
	 "Weibull"},
	{"logistic",	logis_ReadParameters,	logis_Generate,		COMMON_Destroy,		COMMON_Restart,
	 "Logistic"},
	{"loglogistic",	loglogis_ReadParameters,loglogis_Generate,	COMMON_Destroy,		COMMON_Restart,
	 "Log-logistic"},

	// Non parametric
	{"empirical",	emp_ReadParameters,		emp_Generate,		emp_Destroy,		COMMON_Restart,
	 "Empirical"},
	{"replay",		replay_ReadParameters,	replay_Generate,	replay_Destroy,		replay_Restart,
	 "Replay data from a trace"},
	{"histogram",	histo_ReadParameters,	histo_Generate,		histo_Destroy,		COMMON_Restart,
	 "Non-parametric from an histogram"},

	// Discrete
	{"disc",		disc_ReadParameters,	disc_Generate,		disc_Destroy,		COMMON_Restart,
	 "Discrete (from 0 to N-1)"},
	{"dset",		dset_ReadParameters,	dset_Generate,		dset_Destroy,		COMMON_Restart,
	 "Discrete (from a set of values)"},
	{"geom",		geom_ReadParameters,	geom_Generate,		COMMON_Destroy,		COMMON_Restart,
	 "Geometric"},
	{"pois",		pois_ReadParameters,	pois_Generate,		COMMON_Destroy,		COMMON_Restart,
	 "Poisson"},
	{"dunif",		dunif_ReadParameters,	dunif_Generate,		COMMON_Destroy,		COMMON_Restart,
	 "Discrete uniform"},
	{"bern",		bern_ReadParameters,	bern_Generate,		COMMON_Destroy,		COMMON_Restart,
	 "Bernoulli"},

	// Combined
	{"lin",			lin_ReadParameters,		lin_Generate,		choiceLin_Destroy,	choiceLin_Restart,
	 "Linear combination of distributions"},
	{"choice",		choice_ReadParameters,	choice_Generate,	choiceLin_Destroy,	choiceLin_Restart,
	 "Probabilistic choice of a distribution"},

	// END SYMBOL
	{"----", NULL, NULL, NULL, NULL, "----"}
};

