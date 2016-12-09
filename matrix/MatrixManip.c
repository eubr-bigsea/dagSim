
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "Matrix.h"



/**************** Manipulation and (de/)composition *************/

/***** Extraction/Recomposition ***************************/

void GetRow(Vector *V, Matrix *M, int Row)
{
	int i;
	
	if(V->Size != M->Cols) {
	    fprintf(stderr,
	    		"\n Gr(M) Incompatible vector/matrix size\n");
	    return;
	}

	if((M->Encoding == ME_DIRECT) &&
	   (V->Encoding == VE_DIRECT)) {
		for(i = 0; i < V->Size; i++) {
		   V->Data.Direct[i] = M->Data.Direct[i+Row*M->Cols];
		}
	} else {
		for(i = 0; i < V->Size; i++) {
		   WriteVec(V, i, ReadMat(M, Row, i));
		}
	}
}

Vector *GetRowN(Matrix *M, int Row)
{
	Vector *D;
	
	D = CreateVector(M->Cols);
	GetRow(D, M, Row);
	
	return D;
}

void PutRow(Matrix *M, Vector *V, int Row)
{
	int i;
	
	if(V->Size != M->Cols) {
	    fprintf(stderr,
	    		"\n Pr(M) Incompatible vector/matrix size\n");
	    return;
	}

	if((M->Encoding == ME_DIRECT) &&
	   (V->Encoding == VE_DIRECT)) {
		for(i = 0; i < V->Size; i++) {
		   M->Data.Direct[i+Row*M->Cols] = V->Data.Direct[i];
		}
	} else {
		for(i = 0; i < V->Size; i++) {
		   WriteMat(M, Row, i, ReadVec(V, i));
		}
	}
}



void GetCol(Vector *V, Matrix *M, int Col)
{
	int i;
	
	if(V->Size != M->Rows) {
	    fprintf(stderr,
	    		"\n Gc(M) Incompatible vector/matrix size\n");
	    return;
	}

	if((M->Encoding == ME_DIRECT) &&
	   (V->Encoding == VE_DIRECT)) {
		for(i = 0; i < V->Size; i++) {
		   V->Data.Direct[i] = M->Data.Direct[i*M->Cols+Col];
		}
	} else {
		for(i = 0; i < V->Size; i++) {
		   WriteVec(V, i, ReadMat(M, i, Col));
		}
	}
}

Vector *GetColN(Matrix *M, int Col)
{
	Vector *D;
	
	D = CreateVector(M->Rows);
	GetCol(D, M, Col);
	
	return D;
}

void PutCol(Matrix *M, Vector *V, int Col)
{
	int i;
	
	if(V->Size != M->Rows) {
	    fprintf(stderr,
	    		"\n Pc(M) Incompatible vector/matrix size\n");
	    return;
	}

	if((M->Encoding == ME_DIRECT) &&
	   (V->Encoding == VE_DIRECT)) {
		for(i = 0; i < V->Size; i++) {
		   M->Data.Direct[i*M->Cols+Col] = V->Data.Direct[i];
		}
	} else {
		for(i = 0; i < V->Size; i++) {
		   WriteMat(M, i, Col, ReadVec(V, i));
		}
	}
}




void GetDiag(Vector *V, Matrix *M)
{
	int i, Dim;
	
	Dim = M->Rows < M->Cols ? M->Rows : M->Cols;
	
	if(V->Size != Dim) {
	    fprintf(stderr,
	    		"\n Gd(M) Incompatible vector/matrix size\n");
	    return;
	}

	if((M->Encoding == ME_DIRECT) &&
	   (V->Encoding == VE_DIRECT)) {
		for(i = 0; i < V->Size; i++) {
		   V->Data.Direct[i] = M->Data.Direct[i*M->Cols+i];
		}
	} else {
		for(i = 0; i < V->Size; i++) {
		   WriteVec(V, i, ReadMat(M, i, i));
		}
	}
}

Vector *GetDiagN(Matrix *M)
{
	Vector *D;
	
	D = CreateVector(M->Rows < M->Cols ? M->Rows : M->Cols);
	GetDiag(D, M);
	
	return D;
}
void PutDiag(Matrix *M, Vector *V)
{
	int i, Dim;
	
	Dim = M->Rows < M->Cols ? M->Rows : M->Cols;
	
	if(V->Size != Dim) {
	    fprintf(stderr,
	    		"\n Pd(M) Incompatible vector/matrix size\n");
	    return;
	}

	if((M->Encoding == ME_DIRECT) &&
	   (V->Encoding == VE_DIRECT)) {
		for(i = 0; i < V->Size; i++) {
		   M->Data.Direct[i*M->Cols+i] = V->Data.Direct[i];
		}
	} else {
		for(i = 0; i < V->Size; i++) {
		   WriteMat(M, i, i, ReadVec(V, i));
		}
	}
}


//void Split(Matrix **D, int Rs, int nRs,               // Next Step
//                       int Cs, int nCs, Matrix *M);   // Next Step
//Matrix *Combine(int Rs, int nRs,                      // Next Step
//                int Cs, int nCs, Matrix **mS);        // Next Step

/***** Manipulation ***************************************/

void Transpose(Matrix *D, Matrix *M)
{
	int i,j;

	if((D->Cols != M->Rows) || (D->Rows != M->Cols)) {
	    fprintf(stderr,
	    		"\n (M^T) Incompatible matrix size\n");
	    return;
	}
	if((D->Encoding == ME_DIRECT) &&
	   (M->Encoding == ME_DIRECT)) {
		for(i = 0; i < M->Rows; i++) 
		  for(j = 0; j < M->Cols; j++) {
		    D->Data.Direct[i*D->Cols+j] =
		    		M->Data.Direct[j*M->Cols+i];
		  }
	} else {
		for(i = 0; i < M->Rows; i++)
		  for(j = 0; j < M->Cols; j++) {
		    WriteMat(D, i, j, ReadMat(M, j, i));
		  }
	}
}

Matrix *TransposeN(Matrix *M)
{
	Matrix *D;
	
	D = CreateMatrixEnc(M->Cols, M->Rows, M->Encoding);
	Transpose(D, M);
	
	return D;
}

void Invert(Matrix *D, Matrix *M)
{
	Matrix *I;
	
	if((D->Cols != M->Rows) || (D->Rows != M->Cols) ||
		(D->Cols != D->Rows)) {
	    fprintf(stderr,
	    		"\n (M^-1) Incompatible matrix size\n");
	    return;
	}
	I = CreateIdentityMatrix(M->Rows);
	SolveMatMatX(M, D, I);
	DestroyMatrix(I);
}

Matrix *InvertN(Matrix *M)
{
	Matrix *D;
	
	D = CreateMatrixEnc(M->Cols, M->Rows, M->Encoding);
	Invert(D, M);
	
	return D;
}


void InvertAlg(Matrix *D, Matrix *M, int Alg)
{
	Matrix *I;
	
	if((D->Cols != M->Rows) || (D->Rows != M->Cols) ||
		(D->Cols != D->Rows)) {
	    fprintf(stderr,
	    		"\n (M^-1) Incompatible matrix size\n");
	    return;
	}
	I = CreateIdentityMatrix(M->Rows);
	SolveMatMatXAlg(M, D, I, Alg);
	DestroyMatrix(I);
}

Matrix *InvertAlgN(Matrix *M, int Alg)
{
	Matrix *D;
	
	D = CreateMatrixEnc(M->Cols, M->Rows, M->Encoding);
	InvertAlg(D, M, Alg);
	
	return D;
}


