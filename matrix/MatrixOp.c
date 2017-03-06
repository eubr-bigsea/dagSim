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



/**************** Basic Numerical Operations ************************/

/***** Sum & Subtraction **********************************/

void AddVec(Vector *D, Vector *S1, Vector *S2)
{
	int i;
	
	if((D->Size != S1->Size) || (S1->Size != S2->Size)) {
	    fprintf(stderr,
	    		"\n (+V) Incompatible vector size\n");
	    return;
	}
	
	if((D->Encoding == VE_DIRECT) &&
	   (S1->Encoding == VE_DIRECT) &&
	   (S2->Encoding == VE_DIRECT)) {
		for(i = 0; i < S1->Size; i++)
		   D->Data.Direct[i] = S1->Data.Direct[i] + S2->Data.Direct[i];
	} else {
	    for(i = 0; i < S1->Size; i++)
	       WriteVec(D, i, ReadVec(S1, i) + ReadVec(S2, i));
	}
}

Vector *AddVecN(Vector *S1, Vector *S2)
{
	Vector *D;
	
	D = CreateVectorEnc(S2->Size, S2->Encoding);
	AddVec(D, S1, S2);
	
	return D;
}

void SubVec(Vector *D, Vector *S1, Vector *S2)
{
	int i;
	if((D->Size != S1->Size) || (S1->Size != S2->Size)) {
	    fprintf(stderr,
	    		"\n (-V) Incompatible vector size\n");
	    return;
	}
	if((D->Encoding == VE_DIRECT) &&
	   (S1->Encoding == VE_DIRECT) &&
	   (S2->Encoding == VE_DIRECT)) {
		for(i = 0; i < S1->Size; i++)
		   D->Data.Direct[i] = S1->Data.Direct[i] - S2->Data.Direct[i];
	} else {
	    for(i = 0; i < S1->Size; i++)
	       WriteVec(D, i, ReadVec(S1, i) - ReadVec(S2, i));
	}
}

Vector *SubVecN(Vector *S1, Vector *S2)
{
	Vector *D;
	
	D = CreateVectorEnc(S2->Size, S2->Encoding);
	SubVec(D, S1, S2);
	
	return D;
}

 
void AddMat(Matrix *D, Matrix *S1, Matrix *S2)
{
	int i,j;
	
	if((D->Rows != S1->Rows) || (S1->Rows != S2->Rows) ||
	   (D->Cols != S1->Cols) || (S1->Cols != S2->Cols)) {
	    fprintf(stderr,
	    		"\n (+M) Incompatible matrix size\n");
	    return;
	}
	
	if((D->Encoding == ME_DIRECT) &&
	   (S1->Encoding == ME_DIRECT) &&
	   (S2->Encoding == ME_DIRECT)) {
		for(i = 0; i < S1->Rows * S1->Cols; i++)
		   D->Data.Direct[i] = S1->Data.Direct[i] + S2->Data.Direct[i];
	} else if((D->Encoding == ME_SPARSE_R) &&
	   (S1->Encoding == ME_SPARSE_R) &&
	   (S2->Encoding == ME_SPARSE_R)) {
		for(i = 0; i < S1->Rows; i++)
		   AddVec(&D->Data.Sparse[i], &S1->Data.Sparse[i],
		   			      &S2->Data.Sparse[i]);
	} else if((D->Encoding == ME_SPARSE_C) &&
	   (S1->Encoding == ME_SPARSE_C) &&
	   (S2->Encoding == ME_SPARSE_C)) {
		for(i = 0; i < S1->Cols; i++)
		   AddVec(&D->Data.Sparse[i], &S1->Data.Sparse[i],
		   			      &S2->Data.Sparse[i]);
	} else if((D->Encoding == ME_DIAGONAL) &&
	   (S1->Encoding == ME_DIAGONAL) &&
	   (S2->Encoding == ME_DIAGONAL)) {
		AddVec(D->Data.Sparse, S1->Data.Sparse,
		   			      S2->Data.Sparse);
	} else if((D->Encoding == ME_UNITARY) &&
	   (S1->Encoding == ME_UNITARY) &&
	   (S2->Encoding == ME_UNITARY)) {
	   	D->Data.Val = S1->Data.Val + S2->Data.Val;
	} else {
	    for(i = 0; i < S1->Rows; i++)
	        for(j = 0; j < S1->Cols; j++)
	           WriteMat(D, i, j, ReadMat(S1, i, j) + ReadMat(S2, i, j));
	}
}

Matrix *AddMatN(Matrix *S1, Matrix *S2)
{
	Matrix *D;
	
	D = CreateMatrixEnc(S2->Rows, S2->Cols, S2->Encoding);
	AddMat(D, S1, S2);
	
	return D;
}

void SubMat(Matrix *D, Matrix *S1, Matrix *S2)
{
	int i,j;
	
	if((D->Rows != S1->Rows) || (S1->Rows != S2->Rows) ||
	   (D->Cols != S1->Cols) || (S1->Cols != S2->Cols)) {
	    fprintf(stderr,
	    		"\n (-M) Incompatible matrix size\n");
	    return;
	}
	
	if((D->Encoding == ME_DIRECT) &&
	   (S1->Encoding == ME_DIRECT) &&
	   (S2->Encoding == ME_DIRECT)) {
		for(i = 0; i < S1->Rows * S1->Cols; i++)
		   D->Data.Direct[i] = S1->Data.Direct[i] - S2->Data.Direct[i];
	} else if((D->Encoding == ME_SPARSE_R) &&
	   (S1->Encoding == ME_SPARSE_R) &&
	   (S2->Encoding == ME_SPARSE_R)) {
		for(i = 0; i < S1->Rows; i++)
		   SubVec(&D->Data.Sparse[i], &S1->Data.Sparse[i],
		   			      &S2->Data.Sparse[i]);
	} else if((D->Encoding == ME_SPARSE_C) &&
	   (S1->Encoding == ME_SPARSE_C) &&
	   (S2->Encoding == ME_SPARSE_C)) {
		for(i = 0; i < S1->Cols; i++)
		   SubVec(&D->Data.Sparse[i], &S1->Data.Sparse[i],
		   			      &S2->Data.Sparse[i]);
	} else if((D->Encoding == ME_DIAGONAL) &&
	   (S1->Encoding == ME_DIAGONAL) &&
	   (S2->Encoding == ME_DIAGONAL)) {
		SubVec(D->Data.Sparse, S1->Data.Sparse,
		   			      S2->Data.Sparse);
	} else if((D->Encoding == ME_UNITARY) &&
	   (S1->Encoding == ME_UNITARY) &&
	   (S2->Encoding == ME_UNITARY)) {
	   	D->Data.Val = S1->Data.Val - S2->Data.Val;
	} else {
	    for(i = 0; i < S1->Rows; i++)
	        for(j = 0; j < S1->Cols; j++) {
	           WriteMat(D, i, j, ReadMat(S1, i, j) - ReadMat(S2, i, j));
	        }
	}
}

Matrix *SubMatN(Matrix *S1, Matrix *S2)
{
	Matrix *D;
	
	D = CreateMatrixEnc(S2->Rows, S2->Cols, S2->Encoding);
	SubMat(D, S1, S2);
	
	return D;
}

/***** Multiplication *************************************/

void MultVecMat(Vector *D, Vector *V, Matrix *M)
{
	int i,j;
	
	double Sum;
	
	if((D->Size != M->Cols) || (V->Size != M->Rows)) {
	    fprintf(stderr,
	    		"\n (V*M) Incompatible vector/matrix size\n");
	    return;
	}

	if((D->Encoding == VE_DIRECT) &&
	   (V->Encoding == VE_DIRECT) &&
	   (M->Encoding == ME_DIRECT)) {
		for(i = 0; i < M->Cols; i++) {
		   Sum = 0.0;
		   for(j = 0; j < M->Rows; j++)
		   	Sum += M->Data.Direct[j*M->Cols+i] * V->Data.Direct[j];
		   D->Data.Direct[i] = Sum;
		}
	} else if((D->Encoding == VE_DIRECT) &&
	   (V->Encoding == VE_DIRECT) &&
	   (M->Encoding == ME_SPARSE_C)) {
//printf("\n Prod V*M Sparse C\n");
		for(i = 0; i < M->Cols; i++)
		    D->Data.Direct[i] = MultVecVec(&M->Data.Sparse[i], V);
	} else if((D->Encoding == VE_DIRECT) &&
	   (V->Encoding == VE_DIRECT) &&
	   (M->Encoding == ME_SPARSE_R)) {
//printf("\n Prod V*M Sparse R\n");
		for(i = 0; i < M->Rows; i++)
		    MultSumVecScal(D, &M->Data.Sparse[i], V->Data.Direct[i]);
	} else {
		for(i = 0; i < M->Cols; i++) {
		   Sum = 0.0;
		   for(j = 0; j < M->Rows; j++)
		   	Sum += ReadMat(M, j, i) * ReadVec(V,j);
		   WriteVec(D, i, Sum);
		}
	}
}

Vector *MultVecMatN(Vector *V, Matrix *M)
{
	Vector *D;
	
	D = CreateVectorEnc(M->Cols, V->Encoding);
	MultVecMat(D, V, M);
	
	return D;
}

void MultMatVec(Vector *D, Matrix *M, Vector *V)
{
	int i,j;
	double Sum;
	
	if((D->Size != M->Rows) || (V->Size != M->Cols)) {
	    fprintf(stderr,
	    		"\n (M*V) Incompatible vector/matrix size\n");
	    return;
	}
	if((D->Encoding == VE_DIRECT) &&
	   (V->Encoding == VE_DIRECT) &&
	   (M->Encoding == ME_DIRECT)) {
		for(i = 0; i < M->Rows; i++) {
		   Sum = 0.0;
		   for(j = 0; j < M->Cols; j++)
		   	Sum += M->Data.Direct[i*M->Cols+j] * V->Data.Direct[j];
		   D->Data.Direct[i] = Sum;
		}
	} else if((D->Encoding == VE_DIRECT) &&
	   (V->Encoding == VE_DIRECT) &&
	   (M->Encoding == ME_SPARSE_R)) {
//printf("\n Prod M*V Sparse R\n");
		for(i = 0; i < M->Rows; i++)
		    D->Data.Direct[i] = MultVecVec(&M->Data.Sparse[i], V);
	} else {
		for(i = 0; i < M->Rows; i++) {
		   Sum = 0.0;
		   for(j = 0; j < M->Cols; j++)
		   	Sum += ReadMat(M, i, j) * ReadVec(V,j);
		   WriteVec(D, i, Sum);
		}
	}
}

Vector *MultMatVecN(Matrix *M, Vector *V)
{
	Vector *D;
	
	D = CreateVectorEnc(M->Rows, V->Encoding);
	MultMatVec(D, M, V);
	
	return D;
}

void MultMatMat(Matrix *D, Matrix *M1, Matrix *M2)
{
	int i,j,k;
	double Sum;

	if((D->Cols != M2->Cols) || (D->Rows != M1->Rows)
	   			 || (M1->Cols != M2->Rows)) {
	    fprintf(stderr,
	    		"\n (M*M) Incompatible matrix size\n");
	    return;
	}
	if((D->Encoding == ME_DIRECT) &&
	   (M1->Encoding == ME_DIRECT) &&
	   (M2->Encoding == ME_DIRECT)) {
		for(i = 0; i < M1->Rows; i++) 
		  for(j = 0; j < M2->Cols; j++) {
		    Sum = 0.0;
		    for(k = 0; k < M1->Cols; k++) 
		   	Sum += M1->Data.Direct[i*M1->Cols+k] *
		   	       M2->Data.Direct[k*M2->Cols+j];
		    D->Data.Direct[i*D->Cols+j] = Sum;
		  }
	} else if((D->Encoding == ME_SPARSE_R) &&
	   (M1->Encoding == ME_SPARSE_R)) {
		for(i = 0; i < M1->Rows; i++)
		    MultVecMat(&D->Data.Sparse[i], &M1->Data.Sparse[i], M2);
	} else {
		for(i = 0; i < M1->Rows; i++)
		  for(j = 0; j < M2->Cols; j++) {
		    Sum = 0.0;
		    for(k = 0; k < M1->Cols; k++) 
		   	Sum += ReadMat(M1, i, k) * ReadMat(M2, k, j);
		    WriteMat(D, i, j, Sum);
		  }
	}
}

Matrix *MultMatMatN(Matrix *M1, Matrix *M2)     
{
	Matrix *D;
	
	D = CreateMatrixEnc(M1->Rows, M2->Cols, M2->Encoding);
	MultMatMat(D, M1, M2);
	
	return D;
}

double MultVecVec(Vector *V1, Vector *V2)
{
	int i;
	double Sum;
	VectorList *VEl;
	
	if(V1->Size != V2->Size) {
	    fprintf(stderr,
	    		"\n (V*V) Incompatible vector size\n");
	    return 0.0;
	}

	if((V1->Encoding == VE_DIRECT) &&
	   (V2->Encoding == VE_DIRECT)) {
		Sum = 0.0;
		for(i = 0; i < V1->Size; i++) {
			Sum += V1->Data.Direct[i] * V2->Data.Direct[i];
		}
	} else if((V1->Encoding == VE_LIST) &&
	   (V2->Encoding == VE_DIRECT)) {
		Sum = 0.0;
		for(VEl = V1->Data.List; VEl != NULL; VEl = VEl->Next) {
			Sum += VEl->Val * V2->Data.Direct[VEl->El];
		}
	} else {
		Sum = 0.0;
		for(i = 0; i < V1->Size; i++) {
			Sum += ReadVec(V1, i) * ReadVec(V2, i);
		}
	}
	
	return Sum;
}

void MultSumPVecVec(Vector *D, Vector *V1, Vector *V2)
{
	int i;
	VectorList *VEl;
	
	if((V1->Size != V2->Size) || (V1->Size != D->Size)) {
	    fprintf(stderr,
	    		"\n (V*V) Incompatible vector size\n");
	    return;
	}

	if((V1->Encoding == VE_DIRECT) &&
	   (V2->Encoding == VE_DIRECT) &&
	   (D->Encoding == VE_DIRECT)) {
		for(i = 0; i < V1->Size; i++) {
			D->Data.Direct[i] += V1->Data.Direct[i] *
					     V2->Data.Direct[i];
		}
	} else if((V1->Encoding == VE_LIST) &&
	   (V2->Encoding == VE_DIRECT) &&
	   (D->Encoding == VE_DIRECT)) {
		for(VEl = V1->Data.List; VEl != NULL; VEl = VEl->Next) {
			D->Data.Direct[VEl->El] += VEl->Val *
						   V2->Data.Direct[VEl->El];
		}
	} else {
		for(i = 0; i < V1->Size; i++) {
			WriteVec(D, i, ReadVec(V1, i) * ReadVec(V2, i));
		}
	}
}

void MultMatScal (Matrix *D, Matrix *M, double S)
{
	int i,j;

	if((D->Cols != M->Cols) || (D->Rows != M->Rows)) {
	    fprintf(stderr,
	    		"\n (M*S) Incompatible matrix size\n");
	    return;
	}
	if((D->Encoding == ME_DIRECT) &&
	   (M->Encoding == ME_DIRECT)) {
		for(i = 0; i < M->Rows; i++) 
/*Omb*/		  for(j = 0; j < M->Cols; j++) {
		    D->Data.Direct[i*D->Cols+j] =
		    		M->Data.Direct[i*M->Cols+j] * S;
		  }
	} else if((D->Encoding == ME_SPARSE_R) &&
	   (M->Encoding == ME_SPARSE_R)) {
		for(i = 0; i < M->Rows; i++)
		  MultVecScal(&D->Data.Sparse[i], &M->Data.Sparse[i], S);
	} else if((D->Encoding == ME_SPARSE_C) &&
	   (M->Encoding == ME_SPARSE_C)) {
		for(i = 0; i < M->Cols; i++)
		  MultVecScal(&D->Data.Sparse[i], &M->Data.Sparse[i], S);
	} else {
		for(i = 0; i < M->Rows; i++)
		  for(j = 0; j < M->Cols; j++) {
		    WriteMat(D, i, j, ReadMat(M, i, j) * S);
		  }
	}
}

Matrix *MultMatScalN(Matrix *M, double S)     
{
	Matrix *D;
	
	D = CreateMatrixEnc(M->Rows, M->Cols, M->Encoding);
	MultMatScal(D, M, S);
	
	return D;
}

void MultSumVecScal(Vector *D, Vector *V, double S)
{
	int i;
	VectorList *VEl;
	
	if(D->Size != V->Size) {
	    fprintf(stderr,
	    		"\n (D = D+V*S) Incompatible vector size\n");
	    return;
	}

	if((D->Encoding == VE_DIRECT) &&
	   (V->Encoding == VE_DIRECT)) {
		for(i = 0; i < V->Size; i++) {
		   D->Data.Direct[i] += V->Data.Direct[i] * S;
		}
	} else if((V->Encoding == VE_LIST) &&
/***************************************************************************
************* Ho corretto!!! Prima er a la contrario ed ho cambiato!!
************* Se si pianta qualche cosa, la causa potrebbe essere questa !!
****************************************************************************/
	   (D->Encoding == VE_DIRECT)) {
		for(VEl = V->Data.List; VEl != NULL; VEl = VEl->Next) {
			D->Data.Direct[VEl->El] += VEl->Val * S;
		}		
	} else {
		for(i = 0; i < V->Size; i++) {
		   WriteVec(D, i, ReadVec(D, i) + ReadVec(V, i) * S);
		}
	}
}

void MultSumMatScal(Matrix *D, Matrix *M, double S)
{
	int i,j;
	
	if((D->Rows != M->Rows) || (D->Cols != M->Cols)) {
	    fprintf(stderr,
	    		"\n (D=D+M*S) Incompatible matrix size\n");
	    return;
	}
	
	if((D->Encoding == ME_DIRECT) &&
	   (M->Encoding == ME_DIRECT)) {
		for(i = 0; i < M->Rows * M->Cols; i++)
		   D->Data.Direct[i] += M->Data.Direct[i] * S;
	} else if((D->Encoding == ME_SPARSE_R) &&
	   (M->Encoding == ME_SPARSE_R)) {
		for(i = 0; i < M->Rows; i++)
		   MultSumVecScal(&D->Data.Sparse[i], &M->Data.Sparse[i], S);
	} else if((D->Encoding == ME_SPARSE_C) &&
	   (M->Encoding == ME_SPARSE_C)) {
		for(i = 0; i < M->Cols; i++)
		   MultSumVecScal(&D->Data.Sparse[i], &M->Data.Sparse[i], S);
	} else if((D->Encoding == ME_DIAGONAL) &&
	   (M->Encoding == ME_DIAGONAL)) {
		MultSumVecScal(D->Data.Sparse, M->Data.Sparse, S);
	} else if((D->Encoding == ME_UNITARY) &&
	   (M->Encoding == ME_UNITARY)) {
	   	D->Data.Val += M->Data.Val * S;
	} else {
	    for(i = 0; i < M->Rows; i++)
	        for(j = 0; j < M->Cols; j++)
	           WriteMat(D, i, j, ReadMat(D, i, j) + ReadMat(M, i, j) * S);
	}
}

void MultVecScal(Vector *D, Vector *V, double S)
{
	int i;
	VectorList *VEl;
	
	if(D->Size != V->Size) {
	    fprintf(stderr,
	    		"\n (V*S) Incompatible vector/matrix size\n");
	    return;
	}

	if((D->Encoding == VE_DIRECT) &&
	   (V->Encoding == VE_DIRECT)) {
		for(i = 0; i < V->Size; i++) {
		   D->Data.Direct[i] = V->Data.Direct[i] * S;
		}
	} else if(V->Encoding == VE_LIST) {
		for(VEl = V->Data.List; VEl != NULL; VEl = VEl->Next) {
			WriteVec(D, VEl->El, VEl->Val * S);
		}
	} else {
		for(i = 0; i < V->Size; i++) {
		   WriteVec(D, i, ReadVec(V, i) * S);
		}
	}
}

Vector *MultVecScalN(Vector *V, double S)
{
	Vector *D;
	
	D = CreateVectorEnc(V->Size, V->Encoding);
	MultVecScal(D, V, S);
	
	return D;
}

void CopyMat (Matrix *D, Matrix *S)
{
	int i,j;

	if((D->Cols != S->Cols) || (D->Rows != S->Rows)) {
	    fprintf(stderr,
	    		"\n (M=M) Incompatible matrix size\n");
	    return;
	}
	if((D->Encoding == ME_DIRECT) &&
	   (S->Encoding == ME_DIRECT)) {
		for(i = 0; i < S->Rows; i++) 
/*Omb*/		  for(j = 0; j < S->Cols; j++) {
		    D->Data.Direct[i*D->Cols+j] =
		    		S->Data.Direct[i*S->Cols+j];
		  }
	} else if((D->Encoding == ME_SPARSE_R) &&
	   (S->Encoding == ME_SPARSE_R)) {
		for(i = 0; i < S->Rows; i++)
		   CopyVec(&D->Data.Sparse[i], &S->Data.Sparse[i]);
	} else if((D->Encoding == ME_SPARSE_C) &&
	   (S->Encoding == ME_SPARSE_C)) {
		for(i = 0; i < S->Cols; i++)
		   CopyVec(&D->Data.Sparse[i], &S->Data.Sparse[i]);
	} else {
		for(i = 0; i < S->Rows; i++)
		  for(j = 0; j < S->Cols; j++) {
		    WriteMat(D, i, j, ReadMat(S, i, j));
		  }
	}
}

void CopyVec(Vector *D, Vector *S)
{
	int i;
	VectorList *VEl;
	
	if(D->Size != S->Size) {
	    fprintf(stderr,
	    		"\n (V=V) Incompatible vector/matrix size\n");
	    return;
	}
	if((D->Encoding == VE_DIRECT) &&
	   (S->Encoding == VE_DIRECT)) {
		for(i = 0; i < S->Size; i++) {
		   D->Data.Direct[i] = S->Data.Direct[i];
		}
	} else if((D->Encoding == VE_LIST) &&
	   (S->Encoding == VE_LIST)) {
		for(VEl = S->Data.List; VEl != NULL; VEl = VEl->Next)
			WriteVec(D, VEl->El, VEl->Val);
	} else {
		for(i = 0; i < S->Size; i++) {
		   WriteVec(D, i, ReadVec(S, i));
		}
	}
}

