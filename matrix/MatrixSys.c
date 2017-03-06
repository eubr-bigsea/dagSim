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


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "Matrix.h"





/***** Solution algoritms ********************************/

#define ITER_SHOW 10001

double LA_MAX_ITER = 10001;
double ISOL_PRECISION = 10e-12;

int TotalIterations;
double SOROmega = 1.05;

int GetTotalIterations()
{
	return TotalIterations;
}

void SetSOROmega(double Val)
{
	SOROmega = Val;
}

double GetSOROmega()
{
	return SOROmega;
}



void GaussSolve(Matrix *A, Vector *X, Vector *B)
{
	int i, j, k;
	double Diff, Val;
	Vector *V, *V1;
	
	V = DupVector(X);

	for(i = 0; i < LA_MAX_ITER; i++) {
		
		V1 = CreateVector(V->Size);

		for(j = 0; j < V->Size; j++) {
		  Val = ReadVec(B, j);
		  for(k = 0; k < j; k++) {
		    Val -= ReadMat(A, j, k) * ReadVec(V1, k);
		  }
		  for(k = j+1; k < V->Size; k++) {
		    Val -= ReadMat(A, j, k) * ReadVec(V, k);
		  }
		  Val /= ReadMat(A, j, j);
		  WriteVec(V1, j, Val);
		}

		Diff = 0.0;
		for(k = 0; k < V->Size; k++)
		{
			Diff += fabs(ReadVec(V, k) - ReadVec(V1, k));
		}
		
		DestroyVector(V);
		V = V1;

		if(Diff < ISOL_PRECISION) break;
	}
	
	CopyVec(X, V);
	DestroyVector(V);
	TotalIterations = i;	
}




void SORSolve(Matrix *A, Vector *X, Vector *B)
{
	int i, j, k;
	double Diff, Val;
	Vector *V, *V1;
	
	V = DupVector(X);

	for(i = 0; i < LA_MAX_ITER; i++) {
			
		V1 = CreateVector(V->Size);

		for(j = 0; j < V->Size; j++) {
		  Val = ReadVec(B, j);
		  for(k = 0; k < j; k++) {
		    Val -= ReadMat(A, j, k) * ReadVec(V1, k);
		  }
		  for(k = j+1; k < V->Size; k++) {
		    Val -= ReadMat(A, j, k) * ReadVec(V, k);
		  }
		  Val /= ReadMat(A, j, j);
		  WriteVec(V1, j, SOROmega * Val +
		  		  (1.0 - SOROmega) * ReadVec(V, j));
		}

		Diff = 0.0;
		for(k = 0; k < V->Size; k++)
		{
			Diff += fabs(ReadVec(V, k) - ReadVec(V1, k));
		}
		
		DestroyVector(V);
		V = V1;

		if(Diff < ISOL_PRECISION) break;
	}
	
	CopyVec(X, V);
	DestroyVector(V);
	TotalIterations = i;		
}



void GaussSolveT(Vector *X, Matrix *A, Vector *B)
{
	int i, j, k;
	double Diff, Val;
	Vector *V, *V1;
	
	V = DupVector(X);

	if((X->Encoding == VE_DIRECT) &&
	   (B->Encoding == VE_DIRECT) &&
	   (A->Encoding == ME_SPARSE_C)) {

//printf("Optimized! ");		
	   for(i = 0; i < LA_MAX_ITER; i++) {
	   	double CenterEl = 0.0;
		V1 = CreateVectorEnc(V->Size, VE_DIRECT);
		for(j = 0; j < V->Size; j++) {
		  Val = ReadVec(B, j);
		  
		  if(A->Data.Sparse[j].Encoding == VE_LIST ){
		    VectorList *VEl;
		    for(VEl = A->Data.Sparse[j].Data.List; VEl != NULL; VEl = VEl->Next) {
//printf("[%d,%d] ",j, VEl->El);
		      if(VEl->El < j)
		        Val -= V1->Data.Direct[VEl->El] * VEl->Val;
		      else if (VEl->El > j)
		        Val -= V->Data.Direct[VEl->El] * VEl->Val;
		      else
		        CenterEl = VEl->Val;
		    }
		    Val /= CenterEl;
		  } else {
		    for(k = 0; k < j; k++) {
		      Val -= ReadVec(V1, k) * ReadMat(A, k, j);
		    }
		    for(k = j+1; k < V->Size; k++) {
		      Val -= ReadVec(V, k) * ReadMat(A, k, j);
		    }
		    Val /= ReadMat(A, j, j);
		  }
		  V1->Data.Direct[j] = Val;
		}
		Diff = 0.0;
		for(k = 0; k < V->Size; k++)
		{
			Diff += fabs(ReadVec(V, k) - ReadVec(V1, k));
		}
		
		DestroyVector(V);
		V = V1;
//PrintVector(V);
//if(getchar()==27)exit(-1);
		if(Diff < ISOL_PRECISION) break;
	    }

	} else {
	   for(i = 0; i < LA_MAX_ITER; i++) {
		
		V1 = CreateVector(V->Size);

		for(j = 0; j < V->Size; j++) {
		  Val = ReadVec(B, j);
		  for(k = 0; k < j; k++) {
		    Val -= ReadVec(V1, k) * ReadMat(A, k, j);
		  }
		  for(k = j+1; k < V->Size; k++) {
		    Val -= ReadVec(V, k) * ReadMat(A, k, j);
		  }
		  Val /= ReadMat(A, j, j);
		  WriteVec(V1, j, Val);
		}

		Diff = 0.0;
		for(k = 0; k < V->Size; k++)
		{
			Diff += fabs(ReadVec(V, k) - ReadVec(V1, k));
		}
		
		DestroyVector(V);
		V = V1;

		if(Diff < ISOL_PRECISION) break;
	    }
	}
//printf(" Iterations : %d, Prec = %g\n", i, Diff);
	CopyVec(X, V);
	DestroyVector(V);
	TotalIterations = i;	
}




void SORSolveT(Vector *X, Matrix *A, Vector *B)
{
	int i, j, k;
	double Diff, Val;
	Vector *V, *V1;
	
	V = DupVector(X);

	for(i = 0; i < LA_MAX_ITER; i++) {
		
		V1 = CreateVector(V->Size);

		for(j = 0; j < V->Size; j++) {
		  Val = ReadVec(B, j);
		  for(k = 0; k < j; k++) {
		    Val -= ReadVec(V1, k) * ReadMat(A, k, j);
		  }
		  for(k = j+1; k < V->Size; k++) {
		    Val -= ReadVec(V, k) * ReadMat(A, k, j);
		  }
		  Val /= ReadMat(A, j, j);
		  WriteVec(V1, j, SOROmega * Val +
		  		  (1.0 - SOROmega) * ReadVec(V, j));
		}

		Diff = 0.0;
		for(k = 0; k < V->Size; k++)
		{
			Diff += fabs(ReadVec(V, k) - ReadVec(V1, k));
		}
		
		DestroyVector(V);
		V = V1;

		if(Diff < ISOL_PRECISION) break;
	}
	
	CopyVec(X, V);
	DestroyVector(V);
	TotalIterations = i;		
}









Matrix *LUDecomp(Matrix *A)
{
	Matrix *LU;
	int i, j, k;
	double Val;
	
	LU = CreateMatrixEnc(A->Rows, A->Cols, A->Encoding);
	
	for(i = 0; i < A->Rows; i++) {
	  for(j = i; j < A->Cols; j++) {
	    Val = ReadMat(A, i, j);
//printf("%g-_",Val);
	    for(k = 0; k < i; k++) {
	      Val -= ReadMat(LU, i, k) * ReadMat(LU, k, j);
//printf("(%g*%g=%g)",ReadMat(LU, i, k),ReadMat(LU, k, j),ReadMat(LU, i, k) * ReadMat(LU, k, j));
	    }
	    WriteMat(LU, i, j, Val);
//printf("L[%d,%d]=%g\n",i,j,Val);
	  }
	  for(j = i+1; j < A->Rows; j++) {
	    Val = ReadMat(A, j, i);
//printf("%g-_",Val);
	    for(k = 0; k < i; k++) {
	      Val -= ReadMat(LU, j, k) * ReadMat(LU, k, i);
//printf("(%g*%g=%g)",ReadMat(LU, i, k),ReadMat(LU, k, j),ReadMat(LU, i, k) * ReadMat(LU, k, j));
	    }
	    Val /= ReadMat(LU, i, i);
	    WriteMat(LU, j, i, Val);
//printf("/%g_U[%d,%d]=%g\n",ReadMat(LU, i, i),j,i,Val);
	  }
	}
	
	return LU;
}

void LUBackSubst(Matrix *LU, Vector *X, Vector *B)
{

  int i, j;
  Vector *Y;
  double Val;
  
  Y = CreateVector(X->Size);
  for(i = 0; i < X->Size; i++) {
    Val = ReadVec(B, i);
    for(j = 0; j < i; j++) { 
      Val -= ReadMat(LU, i, j) * ReadVec(Y, j);
    }
    WriteVec(Y, i, Val);
  }
  
  for(i = X->Size - 1; i >= 0; i--) {
    Val = ReadVec(Y, i);
    for(j = i + 1; j < X->Size; j++) {
      Val -= ReadMat(LU, i, j) * ReadVec(X, j);
    }
    Val /= ReadMat(LU, i, i);
    WriteVec(X, i, Val);
  }
  
  DestroyVector(Y);
}

void LUFwdSubst(Vector *X, Matrix *LU, Vector *B)
{
  int i, j;
  Vector *Y;
  double Val;
  
  Y = CreateVector(X->Size);
  for(i = 0; i < X->Size; i++) {
    Val = ReadVec(B, i);
    for(j = 0; j < i; j++) { 
      Val -= ReadMat(LU, j, i) * ReadVec(Y, j);
    }
    Val /= ReadMat(LU, i, i);
    WriteVec(Y, i, Val);
  }
  
  for(i = X->Size - 1; i >= 0; i--) {
    Val = ReadVec(Y, i);
    for(j = i + 1; j < X->Size; j++) {
      Val -= ReadMat(LU, j, i) * ReadVec(X, j);
    }
    WriteVec(X, i, Val);
  }
  
  DestroyVector(Y);
}

/***** Linear System Funcs ********************************/

/* Unknown to the right */

void SolveMatVecAlg(Matrix *A, Vector *X, Vector *B, int Alg)
{
	Matrix *LU;
	
	if((A->Rows != A->Cols) || (A->Rows != B->Size) ||
	   (B->Size != X->Size)) {
		fprintf(stderr,
                        "\n (AX=B) Incompatible matrix/Vector size\n");
                return;
        }
        
        switch(Alg) {
	    case LA_LU_DECOMP:
	        LU = LUDecomp(A);
	    	LUBackSubst(LU, X, B);
	    	DestroyMatrix(LU);
	        break;
	    case LA_GAUSS_SEIDEL:
	    	GaussSolve(A, X, B);
	        break;
	    case LA_SOR:
	    	SORSolve(A, X, B);
	        break;
	}
}

void SolveMatVec(Matrix *A, Vector *X, Vector *B)
{
	SolveMatVecAlg(A, X, B, LA_LU_DECOMP);
}




void SolveMatMatXAlg(Matrix *A, Matrix *X, Matrix *B, int Alg)
{
	int i;
	Matrix *LU;
	Vector *iX, *iB;
	
	if((A->Rows != A->Cols) || (A->Rows != B->Rows) ||
	   (B->Cols != X->Cols) || (X->Rows != B->Rows)) {
		fprintf(stderr,
                        "\n (AX=B) Incompatible matrix size\n");
                return;
        }

	switch(Alg) {
	    case LA_LU_DECOMP:
	        LU = LUDecomp(A);
	        iX = CreateVector(B->Rows);
	    	for(i = 0; i < B->Cols; i++) {
	    	    iB = GetColN(B, i);
	    	    LUBackSubst(LU, iX, iB);
	    	    PutCol(X, iX, i);
	    	    DestroyVector(iB);
	    	}
	    	DestroyVector(iX);
	    	DestroyMatrix(LU);
	        break;
	    case LA_GAUSS_SEIDEL:
	        iX = CreateVector(B->Rows);
	    	for(i = 0; i < B->Cols; i++) {
	    	    iB = GetColN(B, i);
	    	    GaussSolve(A, iX, iB);
	    	    PutCol(X, iX, i);
	    	    DestroyVector(iB);
	    	}
	    	DestroyVector(iX);
	        break;
	    case LA_SOR:
	        iX = CreateVector(B->Rows);
	    	for(i = 0; i < B->Cols; i++) {
	    	    iB = GetColN(B, i);
	    	    SORSolve(A, iX, iB);
	    	    PutCol(X, iX, i);
	    	    DestroyVector(iB);
	    	}
	    	DestroyVector(iX);
	        break;
	}
}

void SolveMatMatX(Matrix *A, Matrix *X, Matrix *B)
{
	SolveMatMatXAlg(A, X, B, LA_LU_DECOMP);
}


/* Unknown to the left */

void SolveVecMatAlg(Vector *X, Matrix *A, Vector *B, int Alg)
{
	Matrix *LU;
	
	if((A->Rows != A->Cols) || (A->Rows != B->Size) ||
	   (B->Size != X->Size)) {
		fprintf(stderr,
                        "\n (XA=B) Incompatible matrix/Vector size\n");
                return;
        }
        
        switch(Alg) {
	    case LA_LU_DECOMP:
	        LU = LUDecomp(A);
//PrintMatrixEx(LU, "LU Decompd.");
	    	LUFwdSubst(X, LU, B);
	    	DestroyMatrix(LU);
	        break;
	    case LA_GAUSS_SEIDEL:
	    	GaussSolveT(X, A, B);
	        break;
	    case LA_SOR:
	    	SORSolveT(X, A, B);
	        break;
	}
}

void SolveVecMat(Vector *X, Matrix *A, Vector *B)
{
	SolveVecMatAlg(X, A, B, LA_LU_DECOMP);
}




void SolveMatXMatAlg(Matrix *X, Matrix *A, Matrix *B, int Alg)
{
	int i;
	Matrix *LU;
	Vector *iX, *iB;
	
	if((A->Rows != A->Cols) || (A->Cols != B->Cols) ||
	   (B->Cols != X->Cols) || (X->Rows != B->Rows)) {
		fprintf(stderr,
                        "\n (XA=B) Incompatible matrix size\n");
                return;
        }

	switch(Alg) {
	    case LA_LU_DECOMP:
	        LU = LUDecomp(A);
	        iX = CreateVector(B->Cols);
	    	for(i = 0; i < B->Rows; i++) {
	    	    iB = GetRowN(B, i);
	    	    LUBackSubst(LU, iX, iB);
	    	    PutRow(X, iX, i);
	    	    DestroyVector(iB);
	    	}
	    	DestroyVector(iX);
	    	DestroyMatrix(LU);
	        break;
	    case LA_GAUSS_SEIDEL:
	        iX = CreateVector(B->Cols);
	    	for(i = 0; i < B->Rows; i++) {
	    	    iB = GetRowN(B, i);
	    	    GaussSolveT(iX, A, iB);
	    	    PutRow(X, iX, i);
	    	    DestroyVector(iB);
	    	}
	    	DestroyVector(iX);
	        break;
	    case LA_SOR:
	        iX = CreateVector(B->Cols);
	    	for(i = 0; i < B->Rows; i++) {
	    	    iB = GetRowN(B, i);
	    	    SORSolveT(iX, A, iB);
	    	    PutRow(X, iX, i);
	    	    DestroyVector(iB);
	    	}
	    	DestroyVector(iX);
	        break;
	}
}

void SolveMatXMat(Matrix *X, Matrix *A, Matrix *B)
{
	SolveMatXMatAlg(X, A, B, LA_LU_DECOMP);
}


