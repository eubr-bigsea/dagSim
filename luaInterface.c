#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include <pthread.h>

#include "matrix/Matrix.h"
#include "luaInterface.h"

/** lua errors **/

void error (lua_State *L, const char *fmt, ...) {
	va_list argp;
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
	lua_close(L);
	exit(EXIT_FAILURE);
}

/** Lua data access procedures **/

double readReal(lua_State *L, char *var) {
	double val;
	
	lua_getfield(L, -1, var);

	if (!lua_isnumber(L, -1))
		error(L, "`%s' should be a number\n", var);
	val = lua_tonumber(L, -1);
	lua_pop(L, 1);
	
	return val;
}

double readRealDef(lua_State *L, char *var, double def) {
	double val;
	
	lua_getfield(L, -1, var);

	if (!lua_isnumber(L, -1)) {
		val = def;
	} else {
		val = lua_tonumber(L, -1);
	}
	lua_pop(L, 1);
	
	return val;
}

char *readString(lua_State *L, char *var) {
	const char *val;
	
	lua_getfield(L, -1, var);

	if (!lua_isstring(L, -1))
		error(L, "`%s' should be a string\n", var);
	val = lua_tostring(L, -1);
	lua_pop(L, 1);
	
	return strdup(val);
}


char *readStringDef(lua_State *L, char *var, const char *def) {
	const char *val;
	
	lua_getfield(L, -1, var);

	if (!lua_isstring(L, -1)) {
		val = def;
	} else {
		val = lua_tostring(L, -1);
		lua_pop(L, 1);
	}
		
	return strdup(val);
}


char *readVectorElString(lua_State *L, char *var, int idx) {
	const char *val;
	
	lua_getfield(L, -1, var);
	if (!lua_istable(L, -1))
		error(L, "`%s' should be a table\n", var);

	lua_pushnumber(L, idx+1);
	lua_gettable(L, -2);

	if (!lua_isstring(L, -1))
		error(L, "%s[%d] should be a string\n", var, idx+1);
	val = lua_tostring(L, -1);
	lua_pop(L, 2);
	
	return strdup(val);
}

/* double readMatrixReal(lua_State *L, char *var, int i, int j) {
	double val;
	
	lua_getglobal(L, var);
	if (!lua_istable(L, -1))
		error(L, "`%s' should be a table\n", var);
	lua_pushnumber(L, i+1);
	lua_gettable(L, -2);
	if (!lua_istable(L, -1))
		error(L, "`%s' should have a second dimension\n", var);
	lua_pushnumber(L, j+1);
	lua_gettable(L, -2);

	if (!lua_isnumber(L, -1))
		error(L, "%s[%d,%d] should be a number\n", var, i+1, j+1);
	val = lua_tonumber(L, -1);
	lua_pop(L, 3);
	
	return val;
}
*/
int countElementsTopTable(lua_State *L) {
	int cnt = 0;
	if (!lua_istable(L, -1)) {
		if(lua_isnil(L, -1)) {
			lua_pop(L, 1);
			return 0;
		} else {
			return -1;
		}
	}

	lua_pushnil(L);
	while(lua_next(L, -2) != 0) {
		cnt ++;
		lua_pop(L, 1);
	}
	
	return cnt;
}

int countElements(lua_State *L, char *var) {
	int cnt = 0;
	
	lua_getfield(L, -1, var);
	cnt = countElementsTopTable(L);
	if(cnt < 0) {
		error(L, "`%s' should be a table\n", var);
	}
	lua_pop(L, 1);
	
	return cnt;
}

void getKeyValStrings(lua_State *L, char *var, char **keys, char **vals) {
	// this should be called only after {countElements()}
	int pos = 0;
	lua_getfield(L, -1, var);
	lua_pushnil(L);
	while(lua_next(L, -2) != 0) {
		if(keys != NULL) {
			if(lua_isstring(L, -2)) {
				keys[pos] = strdup(lua_tostring(L, -2));
			} else {
				printf("Cannot read other types of key (%s)\n", var);
				exit(-1);
			}
		}
		if(vals != NULL) {	
			if(lua_isstring(L, -1)) {
				vals[pos] = strdup(lua_tostring(L, -1));
			} else {
				printf("Cannot read other types of values (%s)\n", var);
				exit(-1);
			}
		}
		lua_pop(L, 1);
		pos++;
	}
	lua_pop(L, 1);
}


void pushSubTable(lua_State *L, int idx) {
	lua_pushnumber(L, idx + 1);
	lua_gettable(L, -2);
	if (!lua_istable(L, -1)) {
		printf("Error fectching sub-table\n");
		exit(-1);
	}
}

void setTableElemIdx(lua_State *L, int idx, double val) {
	lua_pushnumber(L, idx);
	lua_pushnumber(L, val);
	lua_settable(L, -3);
} 

void pushFieldTable(lua_State *L, char *idx) {
	lua_pushstring(L, idx);
	lua_gettable(L, -2);
	if (!lua_istable(L, -1)) {
		printf("Error fectching sub-table\n");
		exit(-1);
	}
}


/** lua functions written in C **/

int (*_Analyze)(LuaWrapper *L);

int luaSolve( lua_State *L )
{
   int r;
   LuaWrapper Lw = {L, 0, 1};
   
   lua_getglobal(L, "_G");
   r = _Analyze(&Lw);
   lua_remove(L, 1);
   return r;
}

typedef struct {
	char *inData;		// data to set the global variables for the thread
	int r;				// result of Analyze for the thread
	char *outData;		// results computed by the thread
	int thId;			// id of the thread
	int totThs;			// total number of threads
} threadData;

threadData *configsData;
char *preamble = "__state = function() ";			// context in the new state
char *tailer = "; end; __state = __state();";

#define DEFAULT_CONCURRENCY 4
#define MAX_CONCURRENCY 128
int MaxConcurrency = DEFAULT_CONCURRENCY;			// maximum parallelism

pthread_mutex_t endMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  endCond  = PTHREAD_COND_INITIALIZER;

void *callAnalyzeInThread(void *tdata) {
    LuaWrapper Lw;
	threadData *data = (threadData *)tdata;

	lua_State *Lsub = luaL_newstate();					// creates the sub-context
	luaL_openlibs(Lsub);
	
	luaL_dostring(Lsub, data->inData);			// transfer variables in new context

	lua_getglobal(Lsub, "__state");						// make the new variables global
    lua_pushnil(Lsub);			// copies the configuration parameters
    while(lua_next(Lsub, -2) != 0) {
//printf("Setting %s = %s\n", lua_tostring(Lsub, -2), lua_tostring(Lsub, -1));
		lua_setglobal(Lsub, lua_tostring(Lsub, -2));
    }
    lua_pop(Lsub, 1);			// clears the stack
	lua_pushnil(Lsub);
	lua_setglobal(Lsub, "__state");
   
    lua_getglobal(Lsub, "_G");
    Lw.L = Lsub; Lw.thId = data->thId; Lw.totThs = data->totThs;
    data->r = _Analyze(&Lw);			// solve the model
    lua_remove(Lsub, 1);

    lua_setglobal(Lsub, "__res");
    luaL_dostring(Lsub, "__s = require(\"serpent\"); __state = __s.dump(__res);");
    lua_getglobal(Lsub, "__state");
    const char *results = lua_tostring(Lsub, -1);
    data->outData = malloc(strlen(preamble) + strlen(tailer) + strlen(results));
    sprintf(data->outData, "%s%s%s", preamble, results, tailer);

    lua_close(Lsub);

//printf("Thread <%s> has finished\n", data->inData);
	// signal that a thread has ended
    pthread_mutex_lock(&endMutex);
    pthread_cond_signal(&endCond);
    pthread_mutex_unlock(&endMutex);	
    
    
   	free(data->inData);

    return NULL;
}

int luaMultiSolve( lua_State *L )
{
   int r = 1, i, n, t;

   if(!lua_istable(L, 1)) {		// check the parameters are passed to the multisolve
   		printf("Error in multisolve: a table of configurations must be passed!\n");
   		exit(-1);
   }
   lua_len(L, 1);
   n = lua_tointeger(L, -1);
   lua_pop(L, 1);
   if(n < 1) {
   		printf("Error in multisolve: at least a configuration must be passed!\n");
   		exit(-1);
   }
   
   lua_getglobal(L, "_exe");
   lua_getfield(L, -1, "maxThreads");
   if(!lua_isnil(L, -1)) {
   		MaxConcurrency = lua_tointeger(L, -1);
   		if((MaxConcurrency < 1) || (MaxConcurrency > MAX_CONCURRENCY)) {
   			MaxConcurrency = DEFAULT_CONCURRENCY;
   		}
   } else {
   	    MaxConcurrency = DEFAULT_CONCURRENCY;
   }
   lua_pop(L, 2);
   
   configsData = calloc(n, sizeof(threadData));
   
   // Prepares the configuration structures for threaded execution
   
   lua_newtable(L);				// to contain the merged results
   lua_getglobal(L, "_G");
   for(i = 0; i < n; i++) {
	   t = lua_gettop(L);
	   
	   lua_rawgeti(L, 1, i+1);	// check the configuration
	   if(!lua_istable(L, 1)) {
	   		printf("Error in multisolve: configuration %i is not a table.\n", i);
	   		exit(-1);
	   }
	   
	   lua_pushnil(L);			// copies the configuration parameters
	   while(lua_next(L, -2) != 0) {
			lua_pushstring(L,lua_tostring(L, -2));
			lua_insert(L, -2);
	   		lua_settable(L, t);
	   }
	   lua_pop(L, 1);

	   // copies the configuration to a new lua state
		luaL_dostring(L, "__state = solver.serialize()");	// serializes the context
		lua_getglobal(L, "__state");

		// creates a function to recreate the
		const char *params = lua_tostring(L, -1);
		configsData[i].inData = malloc(strlen(preamble) + strlen(tailer) + strlen(params));
		configsData[i].thId = i;
		configsData[i].totThs = n;
		sprintf(configsData[i].inData, "%s%s%s", preamble, params, tailer);
//printf("%s \n", stateBuilder);
		lua_pop(L,1);										// clears the stack
		lua_pushnil(L);
		lua_setglobal(L, "__state");
   }

// 128 threads is the P7 with 4 cpus, 32 cores and 128 SMT.
   pthread_t *threads = calloc(n, sizeof(pthread_t));
   int rc;
   void *status;

	// executes threads
   for(i = 0; i < n; i++) {
   	  if(i >= MaxConcurrency) {	// if already as many threads as cores are running
	  // waits for a thread to end before starting a new one
	      pthread_mutex_lock(&endMutex);
	      pthread_cond_wait(&endCond, &endMutex);
	      pthread_mutex_unlock(&endMutex);
   	  }
//printf("Starting thread:%d\n", i+1);
//		callAnalyzeInThread((void *)&configsData[i]);
      rc = pthread_create(&threads[i], NULL, callAnalyzeInThread, (void *)&configsData[i]);
      if (rc){
         printf("ERROR; return code from pthread_create() for thread %d is %d\n", i, rc);
         exit(-1);
      }	  
   }
   
   // waits for all thread to finish
   for(i = 0; i < n; i++) {
      rc = pthread_join(threads[i], &status);
      if (rc) {
         printf("ERROR; return code from pthread_join() for thread %d is %d\n", i, rc);
         exit(-1);
         }

   }
   
   // merges results in the main thread
   for(i = 0; i < n; i++) {
	    r = r & configsData[i].r;

		luaL_dostring(L, configsData[i].outData);		// transfer the results in the old context
		free(configsData[i].outData);

		lua_getglobal(L, "__state");
	    lua_rawseti(L, 2, i+1);	// output results
		lua_pushnil(L);
		lua_setglobal(L, "__state");
	    
   }
   lua_remove(L, t);
   lua_remove(L, 1);
   
   free(configsData);
   free(threads);
   
   return r;
}

const struct luaL_Reg luaInterfaceFunc[] =
{
  { "solve", luaSolve   },
  { "multisolve", luaMultiSolve   },
  { NULL, NULL }
};


/** Base Lua script execution functions */


void loadLua(lua_State *L, char *filename) {	
	if (luaL_loadfile(L, filename) || lua_pcall(L, 0, 0, 0))
	  error(L, "cannot run LUA file: %s\n %s\n", filename,
	           lua_tostring(L, -1));
	
}

lua_State *initLuaEnvironment(char *envFile, int argc, char *argv[]) {
	int i;
	lua_State *L = luaL_newstate();

	luaL_openlibs(L);
	lua_newtable(L);
	luaL_setfuncs(L, luaInterfaceFunc, 0);	// sets the C functions in the lua environment
	
	lua_pushstring(L, "args"); // Adds a new field called "args" to the "_exe" object.
	lua_newtable(L);
	 
	for(i = 0; i < argc; i++) {			// copies the command line arguments in the args vector
	 	lua_pushstring(L, argv[i]);
	 	lua_rawseti(L, -2, i+1);
	}
	lua_settable(L, -3);
	
	lua_setglobal(L, "_exe"); 		// sets the "_exe" object in lua
	loadLua(L, envFile);
	           
	return L;
}

void closeLuaEnvironment(lua_State *L) {
	lua_close(L);
}

void LuaLoadAndSolve(char *envFile, char *pkg, char *smain, int argc, char *argv[], int (*AnalyzeFunc)(LuaWrapper *L)) {
	char *name = argv[1];
	_Analyze = AnalyzeFunc;
	
	lua_State *L = initLuaEnvironment(envFile, argc, argv);
	loadLua(L, name);
	
//printf("Stacksize: %d\n", lua_gettop(L));
	lua_getglobal(L, pkg);
	lua_getfield(L, -1, smain);
	if (lua_pcall(L, 0, 0, 0))
	  error(L, "Error %s\n", lua_tostring(L, -1));
	lua_pop(L, 1);
//printf("Stacksize: %d\n", lua_gettop(L));
	
	closeLuaEnvironment(L);
}

