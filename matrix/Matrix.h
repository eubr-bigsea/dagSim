
/******************* Global Constants *********************/

#define MAX_DIRECT_DIM   100
    /* matrix with R*C > MAX_DIRECT_DIM are stored in sparse form */

#define MAX_VEC_DIRECT_DIM 100
   /* Vector is stored in sparse form if Size > MAX_VEC_DIRECT_DIM */

/******************* Matrix Encoding types ****************/

#define ME_ZERO		0
#define ME_DIRECT	1
#define ME_SPARSE_R	2
#define ME_SPARSE_C	3
#define ME_DIAGONAL	4
#define ME_UNITARY	5

#define ME_SPARSE_R_DIRECT_ROWS		102

/******************* Sparse Matrix Encoding ***************/

#define VE_ZERO		0
#define VE_LIST		1
#define VE_RB_TREE	2
#define VE_DIRECT	3


typedef struct VectorList_tag {
	int El;			/* Element index */
	double Val;		/* Element value */
	
	struct VectorList_tag *Next;
				/* Pointer to the next element */
} VectorList;


typedef struct VectorRBTree_tag {
	int El;			/* Element index */
	double Val;		/* Element value */
	
	struct VectorRBTree_tag	*P;	/* Parent node */
	struct VectorRBTree_tag	*Left;	/* Left son node */
	struct VectorRBTree_tag	*Right;	/* Right son node */
	char Color;		/* Color of the node : 0 red, 1 black */
} VectorRBTree;


typedef struct {
	short Encoding;		/* Row Encoding */
	int NonZeroEl;  /* Number of NonZeroElements */
	int Size;		/* Size of vector */
	
	union {
		VectorList *List;
		VectorRBTree *RBTree;
		double *Direct;
	} Data;
} Vector;

/******************* Flags ********************************/

#define MF_HAS_PERMUTATION	1

/******************* Matrix Definition ********************/

typedef struct {
	int Rows;	/* Number of Rows */
	int Cols;	/* Number of Columns */
	
	short Encoding;	/* Matrix Encoding */

	int NonZeroEl;  /* Number of NonZeroElements */
	
	int Flags;	/* Matrix Flags */
	int *Permut;	/* Row permuation */
	
	union {
	   double *Direct;
	   Vector *Sparse;
	   double Val;
	} Data;
} Matrix;


/***************** Prototypes *****************************/
/**********************************************************/
/***** Matrix Allocation **********************************/

Matrix *CreateMatrixEnc(int Rows, int Cols, int Encoding);
Matrix *CreateMatrix(int Rows, int Cols);
void DestroyMatrix(Matrix *M);

Matrix *DupMatrix(Matrix *S);
Matrix *CreateIdentityMatrix(int Size);

/***** Vector Allacation **********************************/

void InitializeVector(Vector *V, int Elem, int Enc);
Vector *CreateVectorEnc(int Elem, int Enc);
Vector *CreateVector(int Elem);
void DeinitializeVector(Vector *V);
void DestroyVector(Vector *V);

Vector *DupVector(Vector *S);

/***** Reading and writing Matrix ****(in Matrix.c)*********************/

void WriteMat(Matrix *M, int R, int C, double Val);
double ReadMat(Matrix *M, int R, int C);

#define WM(A,B,C,D) WriteMat(A,B,C,D)
#define RM(A,B,C)   ReadMat(A,B,C)

/***** Reading and writing Vector ****(in Matrix.c)*********************/

void WriteVec(Vector *V, int El, double Val);
double ReadVec(Vector *M, int El);

#define WV(A,B,C) WriteVec(A,B,C)
#define RV(A,B)   ReadVec(A,B)

/***** Printing ****(in Matrix.c)***************************************/

void fPrintMatrix(FILE *Fp, Matrix *M);
void PrintMatrix(Matrix *M);
void PrintMatrixEx(Matrix *M, char *Name);
void PrintMatrixSPicture(Matrix *M);
void fPrintVector(FILE *Fp, Vector *V);
void PrintVector(Vector *V);






/***** Sum & Subtraction ****(in MatrixOp.c)*****************************/

void AddVec(Vector *D, Vector *S1, Vector *S2);
Vector *AddVecN(Vector *S1, Vector *S2);
void SubVec(Vector *D, Vector *S1, Vector *S2);
Vector *SubVecN(Vector *S1, Vector *S2);

void AddMat(Matrix *D, Matrix *S1, Matrix *S2);
Matrix *AddMatN(Matrix *S1, Matrix *S2);
void SubMat(Matrix *D, Matrix *S1, Matrix *S2);
Matrix *SubMatN(Matrix *S1, Matrix *S2);

/***** Multiplication ****(in MatrixOp.c)******************************/

void MultVecMat(Vector *D, Vector *V, Matrix *M);
Vector *MultVecMatN(Vector *V, Matrix *M);
void MultMatVec(Vector *D, Matrix *M, Vector *V);
Vector *MultMatVecN(Matrix *M, Vector *V);

void MultMatMat(Matrix *D, Matrix *M1, Matrix *M2);
Matrix *MultMatMatN(Matrix *M1, Matrix *M2);

double MultVecVec(Vector *V1, Vector *V2);
void MultSumPVecVec(Vector *D, Vector *V1, Vector *V2);

void MultMatScal(Matrix *D, Matrix *M, double S);
Matrix *MultMatScalN(Matrix *M, double S);

void MultVecScal(Vector *D, Vector *V, double S);
Vector *MultVecScalN(Vector *V, double S);

void MultSumVecScal(Vector *D, Vector *V, double S);
void MultSumMatScal(Matrix *D, Matrix *M, double S);

void CopyMat(Matrix *D, Matrix *S);
void CopyVec(Vector *D, Vector *S);






/***** Extraction/Recomposition ****(in MatrixManip.c)*****************/

void GetRow(Vector *V, Matrix *M, int Row);
Vector *GetRowN(Matrix *M, int Row);
void PutRow(Matrix *M, Vector *V, int Row);

void GetCol(Vector *V, Matrix *M, int Col);
Vector *GetColN(Matrix *M, int Col);
void PutCol(Matrix *M, Vector *V, int Col);

void GetDiag(Vector *V, Matrix *M);
Vector *GetDiagN(Matrix *M);
void PutDiag(Matrix *M, Vector *V);

//void Split(Matrix **D, int Rs, int nRs,		// Next Step
//			 int Cs, int nCs, Matrix *M);	// Next Step
//Matrix *Combine(int Rs, int nRs,			// Next Step
//		  int Cs, int nCs, Matrix **mS);	// Next Step

/***** Manipulation ****(in MatrixManip.c)*******************************/

void Transpose(Matrix *D, Matrix *M);
Matrix *TransposeN(Matrix *M);

void Invert(Matrix *D, Matrix *M);
Matrix *InvertN(Matrix *M);

void InvertAlg(Matrix *D, Matrix *M, int Alg);
Matrix *InvertAlgN(Matrix *M, int Alg);







/***** Linear System Funcs ****(in MatrixSys.c)************************/

#define LA_LU_DECOMP		1
#define LA_GAUSS_SEIDEL		2
#define LA_SOR			3

/* Unknown to right */

void SolveMatVecAlg(Matrix *A, Vector *X, Vector *B, int Alg);
void SolveMatVec(Matrix *A, Vector *X, Vector *B);

void SolveMatMatXAlg(Matrix *A, Matrix *X, Matrix *B, int Alg);
void SolveMatMatX(Matrix *A, Matrix *X, Matrix *B);

/* Unknown to left */

void SolveVecMatAlg(Vector *X, Matrix *A, Vector *B, int Alg);
void SolveVecMat(Vector *X, Matrix *A, Vector *B);

void SolveMatXMatAlg(Matrix *X, Matrix *A, Matrix *B, int Alg);
void SolveMatXMat(Matrix *X, Matrix *A, Matrix *B);

/* Paramters and other */

void SetSOROmega(double Val);
double GetSOROmega();
int GetTotalIterations();
Matrix *LUDecomp(Matrix *A);
void LUFwdSubst(Vector *X, Matrix *LU, Vector *B);
void LUBackSubst(Matrix *LU, Vector *X, Vector *B);



/***** Matrix Exponential ****(in MatrixExp.c)************************/
void MatExpAndIntN(Matrix *Q, double t, Matrix **Exp, Matrix **Int);
void MatExpAndInt2N(Matrix *Q, double t, Matrix **Exp, Matrix **Int);
Matrix **MultVecMatExpTsN(Matrix *P0, Matrix *Q, Vector *T);

