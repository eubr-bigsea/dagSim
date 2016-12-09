#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "matrix/Matrix.h"
#include "luaInterface.h"
#include "distributions.h"



double Rnd() {
// replace with Merseen-Twister
	return drand48();
}

void SetSeed(int seed) {
	srand48(seed);
}

void SetRandomSeed() {
	srand48(time(NULL));
}


void SetRandomSeedTh(int i, int n) {
	srand48(time(NULL)*n+i);
}
