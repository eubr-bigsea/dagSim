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

#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"

// the standard procedures to simplify interfacing with lua

typedef struct {
	lua_State *L;
	int thId;
	int totThs;
} LuaWrapper;

double readReal(lua_State *L, char *var);
double readRealDef(lua_State *L, char *var, double def);
char *readString(lua_State *L, char *var);
char *readStringDef(lua_State *L, char *var, const char *def);
char *readVectorElString(lua_State *L, char *var, int idx);
int countElements(lua_State *L, char *var);
int countElementsTopTable(lua_State *L);
void getKeyValStrings(lua_State *L, char *var, char **keys, char **vals);
int Analyze(LuaWrapper *);
void loadLua(lua_State *L, char *filename);
void LuaLoadAndSolve(char *envFile, char *pkg, char *smain, int argc, char *argv[], int (*AnalyzeFunc)(LuaWrapper *));
void pushSubTable(lua_State *L, int idx);
void pushFieldTable(lua_State *L, char *idx);
void setTableElemIdx(lua_State *L, int idx, double val);
