
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
