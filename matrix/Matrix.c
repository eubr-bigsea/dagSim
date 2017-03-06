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

int MatDebugActive = 0;
#define IMD if(MatDebugActive==1)
#define IMP(A) if(MatDebugActive==1){printf("%s\n",A);fflush(stdout);}

/**************** Generic functions ************************/

/***** Matrix Allocation **********************************/

Matrix *CreateMatrixEnc(int Rows, int Cols, int Encoding)
{
	Matrix *M;
	int i;
IMD {printf("\nRows %d, Cols %d, Enc %d \n", Rows, Cols, Encoding);}
	
	M = malloc(sizeof(Matrix));
	if(M == NULL) {
		fprintf(stderr, "\n Error allocating matrix \n");
		return NULL;
	}
	
	M->Rows = Rows;
	M->Cols = Cols;
	M->NonZeroEl = 0;
	
	M->Encoding = Encoding;
	
	switch(Encoding) {
	    case ME_DIRECT:
IMP("Alloco direttamente")
	    	M->Data.Direct = calloc(Rows * Cols, sizeof(double));
		if(M->Data.Direct == NULL) {
			fprintf(stderr, "\n Error allocating matrix \n");
			return NULL;
		}
IMP("Azzero.....")
	    	for(i = 0; i < Rows * Cols; i++)
	    		M->Data.Direct[i] = 0.0;
	    	break;
	    case ME_SPARSE_R:
	    	M->Data.Sparse = calloc(Rows, sizeof(Vector));
		if(M->Data.Sparse == NULL) {
			fprintf(stderr, "\n Error allocating matrix \n");
			return NULL;
		}
	    	for(i = 0; i < Rows; i++)
	    		InitializeVector(&M->Data.Sparse[i],
	    				Cols, VE_LIST);
	    	break;
	    case ME_SPARSE_R_DIRECT_ROWS:
	    	M->Encoding = ME_SPARSE_R;
	    	M->Data.Sparse = calloc(Rows, sizeof(Vector));
		if(M->Data.Sparse == NULL) {
			fprintf(stderr, "\n Error allocating matrix \n");
			return NULL;
		}
	    	for(i = 0; i < Rows; i++)
	    		InitializeVector(&M->Data.Sparse[i],
	    				Cols, VE_DIRECT);
	    	break;
	    case ME_SPARSE_C:
	    	M->Data.Sparse = calloc(Cols, sizeof(Vector));
		if(M->Data.Sparse == NULL) {
			fprintf(stderr, "\n Error allocating matrix \n");
			return NULL;
		}
	    	for(i = 0; i < Cols; i++)
	    		InitializeVector(&M->Data.Sparse[i],
	    				Rows, VE_LIST);
	    	break;
	    case ME_ZERO:
	    	break;
	    case ME_UNITARY:
	    	M->Data.Val = 0.0;
	    	break;
	    case ME_DIAGONAL:
	        {
	           int Dsize;
	           
	           Dsize = (Rows < Cols ? Rows : Cols);
	           
	    	   M->Data.Sparse = CreateVectorEnc(Dsize, VE_LIST);
	    	}
	    	break;
	    default:
	    	fprintf(stderr,
	    		"\n (A)Encoding not supported: %d \n", Encoding);
	    	return NULL;
	    	break;
	}
	
	return M;
}

Matrix *CreateMatrix(int Rows, int Cols)
{
//|?.	if(Rows * Cols < MAX_DIRECT_DIM) {
		return CreateMatrixEnc(Rows, Cols, ME_DIRECT);
//|?.	} else if(Cols < Rows) {
//|?.		return CreateMatrixEnc(Rows, Cols, ME_SPARSE_R);		
//|?.	} else {
//|?.		return CreateMatrixEnc(Rows, Cols, ME_SPARSE_C);
//|?.	}
	
//|?.	return NULL;
}

void DestroyMatrix(Matrix *M)
{
	int i;
	
	switch(M->Encoding) {
	    case ME_DIRECT:
	    	free(M->Data.Direct);
	    	break;
	    case ME_DIAGONAL:
	    	DestroyVector(M->Data.Sparse);
	    	break;
	    case ME_SPARSE_R:
	    	for(i = 0; i < M->Rows; i++)
	    	    DeinitializeVector(&M->Data.Sparse[i]);
	    	free(M->Data.Sparse);
	    	break;
	    case ME_SPARSE_C:
	    	for(i = 0; i < M->Cols; i++)
	    	    DeinitializeVector(&M->Data.Sparse[i]);
	    	free(M->Data.Sparse);
	    	break;
	    case ME_ZERO:
	    case ME_UNITARY:
	    	break;
	    default:
	    	fprintf(stderr,
	    		"\n (F)Encoding not supported: %d \n", M->Encoding);
	    	return;
	    	break;
	}
	
	free(M);
}

Matrix *DupMatrix(Matrix *M)
{
	Matrix *D;
	
	if(M->Encoding == ME_SPARSE_R) {
	    if(M->Data.Sparse[0].Encoding == VE_DIRECT)
		D = CreateMatrixEnc(M->Rows, M->Cols, ME_SPARSE_R_DIRECT_ROWS);
	    else
		D = CreateMatrixEnc(M->Rows, M->Cols, M->Encoding);
	} else
		D = CreateMatrixEnc(M->Rows, M->Cols, M->Encoding);
	
	CopyMat(D, M);
	
	return D;
}

Matrix *CreateIdentityMatrix(int Size)
{
	Matrix *D;
	
	D = CreateMatrixEnc(Size, Size, ME_UNITARY);
	D->Data.Val = 1.0;
	
	return D;
}

/***** Vector Allacation **********************************/

void InitializeVector(Vector *V, int Size, int Enc)
{
	V->Encoding = Enc;
	V->Size = Size;
	V->NonZeroEl = 0;
	
	switch(Enc) {
	    case VE_ZERO:
	    	break;
	    case VE_LIST:
	    	V->Data.List = NULL;
	    	break;
	    case VE_RB_TREE:
	    	V->Data.RBTree = NULL;
	    	break;
	    case VE_DIRECT:
	    	V->Data.Direct = calloc(sizeof(double), Size);
		if(V->Data.Direct == NULL) {
			fprintf(stderr, "\n Error allocating vector \n");
			return;
		}
	    	break;
	    default:
	    	fprintf(stderr,
	    		"\n (Av)Encoding not supported: %d \n", Enc);
	    	break;
	}
}

Vector *CreateVectorEnc(int Size, int Enc)
{
	Vector *Vec;
	
	Vec = malloc(sizeof(Vector));
	if(Vec == NULL) {
		fprintf(stderr, "\n Error allocating vector \n");
		return NULL;
	}
	InitializeVector(Vec, Size, Enc);
	
	return Vec;
}

Vector *CreateVector(int Size)
{
//|?.	if(Size > MAX_VEC_DIRECT_DIM)
//|?.		return(CreateVectorEnc(Size, VE_LIST));
//|?.	else
		return(CreateVectorEnc(Size, VE_DIRECT));
}

void DeinitializeVector(Vector *V)
{
	switch(V->Encoding) {
	    case VE_ZERO:
	    	break;
	    case VE_LIST:
	    	{
	    	    VectorList *El, *Rem;

	    	    El = V->Data.List;	    	    
	    	    while(El != NULL) {
	    	    	Rem = El;
	    	    	El = El->Next;
	    	    	
	    	    	free(Rem);
	    	    } 
	    	}
	    	break;
	    case VE_RB_TREE:
	    	if(V->Data.RBTree != NULL) {
	    	}
	    	break;
	    case VE_DIRECT:
	    	free(V->Data.Direct);
	    	break;
	    default:
	    	fprintf(stderr,
	    		"\n (Dv)Encoding not supported: %d \n", V->Encoding);
	    	break;
	}
}

void DestroyVector(Vector *V)
{
	DeinitializeVector(V);
	free(V);
}


Vector *DupVector(Vector *V)
{
	Vector *D;
	
	D = CreateVectorEnc(V->Size, V->Encoding);

	CopyVec(D, V);
		
	return D;
}



/***** Reading and writing Vector ***************************/

void WriteVec(Vector *V, int El, double Val)
{
    	VectorList *VEl;

	if(El >= V->Size) {
	    fprintf(stderr,
	    		"\n Writing out of bound\n");
	    return;
	}
	switch(V->Encoding) {
	    case VE_ZERO:
	    	fprintf(stderr,
	    		"\n (Wv)Cannot write Zero vector \n");
	    	return;
	    	break;
	    case VE_LIST:
	    	if((VEl = V->Data.List) == NULL)
	    	{
	    	   VEl = malloc(sizeof(VectorList));
		   if(VEl == NULL) {
		       fprintf(stderr, "\n Error allocating vector \n");
		       return;
		   }

	    	   V->Data.List = VEl;
	    	   VEl->El = El;
	    	   VEl->Val = Val;
	    	   VEl->Next = NULL;
	    	} else {
	    	   while(VEl != NULL) {
	    	   	if(VEl->El == El) {
	    	   	   VEl->Val = Val;
	    	   	   break;
	    	   	}
	    	   	VEl = VEl->Next;
	    	   }
	    	   if(VEl == NULL) {
	    	     VEl = malloc(sizeof(VectorList));
		     if(VEl == NULL) {
		       fprintf(stderr, "\n Error allocating vector \n");
		       return;
		     }

	    	     VEl->El = El;
	    	     VEl->Val = Val;
	    	     VEl->Next = V->Data.List;
	    	     V->Data.List = VEl;
	    	   }
	    	}
	        break;
	    case VE_RB_TREE:
	    	break;
	    case VE_DIRECT:
	    	V->Data.Direct[El] = Val;
	    	break;
	    default:
	    	fprintf(stderr,
	    		"\n (Wv)Encoding not supported: %d \n", V->Encoding);
	    	return;
	    	break;
	}
}


double ReadVec(Vector *V, int El)
{
    	VectorList *VEl;

	if(El >= V->Size) {
	    fprintf(stderr,
	    		"\n Reading out of bound\n");
	    return 0.0;
	}
	switch(V->Encoding) {
	    case VE_ZERO:
		return 0.0;
	    	break;
	    case VE_LIST:
	    	if((VEl = V->Data.List) == NULL)
	    	    return 0.0;
	    	else {
	    	   while(VEl != NULL) {
	    	   	if(VEl->El == El)
	    	   	   return(VEl->Val);
	    	   	VEl = VEl->Next;
	    	   }
	    	   return 0.0;
	    	}
	        break;
	    case VE_RB_TREE:
	    	break;
	    case VE_DIRECT:
	    	return V->Data.Direct[El];
	    	break;
	    default:
	    	fprintf(stderr,
	    		"\n (Rv)Encoding not supported: %d \n", V->Encoding);
	    	return 0.0;
	    	break;
	}
	return 0.0;
}


/***** Reading and writing Matrix ***************************/

void WriteMat(Matrix *M, int R, int C, double Val)
{
	if((R >= M->Rows) || (C >= M->Cols)) {
	    fprintf(stderr,
	    		"\n Writing out of bound\n");
	    return;
	}
	switch(M->Encoding) {
	    case ME_UNITARY:
	    case ME_ZERO:
	    	fprintf(stderr,
	    		"\n (Wm)Cannot write Zero or Unitary Matrix \n");
	    	return;
	    	break;
	    case ME_DIRECT:
//IMD{printf(" Scrivo di %x, (array %x) a (%d,%d) [%d] = %f\n",
//(int)M, (int)M->Data.Direct, R, C, R*M->Cols + C, Val);}
	    	M->Data.Direct[R * M->Cols + C] = Val;
	    	break;
	    case ME_SPARSE_R:
	    	WriteVec(&M->Data.Sparse[R], C, Val);
	    	break;
	    case ME_SPARSE_C:
	    	WriteVec(&M->Data.Sparse[C], R, Val);
	    	break;
	    case ME_DIAGONAL:
	        if(R != C) {
	    	    fprintf(stderr,
	    	   "\n (Wm)Cannot write non diagonal elem. of Diag Matrix \n");
	        } else {
	    	  WriteVec(M->Data.Sparse, R, Val);
	    	}
	    	break;
	    default:
	    	fprintf(stderr,
	    		"\n (Wm)Encoding not supported: %d \n", M->Encoding);
	    	return;
	    	break;
	}
}

double ReadMat(Matrix *M, int R, int C)
{
	if((R >= M->Rows) || (C >= M->Cols)) {
	    fprintf(stderr,
	    		"\n Reading out of bound\n");
	    return 0.0;
	}
	switch(M->Encoding) {
	    case ME_DIRECT:
	    	return M->Data.Direct[R * M->Cols + C];
	    	break;
	    case ME_SPARSE_R:
	    	return ReadVec(&M->Data.Sparse[R], C);
	    	break;
	    case ME_SPARSE_C:
	    	return ReadVec(&M->Data.Sparse[C], R);
	    	break;
	    case ME_ZERO:
	    	return 0.0;
	    	break;
	    case ME_UNITARY:
	    	if(R == C) return M->Data.Val;
	    	else return 0.0;
	    	break;
	    case ME_DIAGONAL:
	    	if(R == C) return ReadVec(M->Data.Sparse, R);
	    	else return 0.0;
	    	break;
	    default:
	    	fprintf(stderr,
	    		"\n (Rm)Encoding not supported: %d \n", M->Encoding);
	    	return 0.0;
	    	break;
	}
	
	return 0.0;
}

/***** Printing *******************************************/

void PrintMatrixSPicture(Matrix *M)
{
	int i, j;
	char Pic;
	double V;
	
	for(i = 0; i < M->Rows; i++) {
	   printf("|");
	   for(j = 0; j < M->Cols; j++) {
	   	V = ReadMat(M, i, j);
	   	Pic = (V > 0.0 ? '+' : (V < 0.0 ? '-' : ' '));
	       printf("%c", Pic);
	   }
	   printf("|\n");
	}
	printf("\n");
}

void fPrintMatrix(FILE *Fp, Matrix *M)
{
	int i, j;
	
	for(i = 0; i < M->Rows; i++) {
	   for(j = 0; j < M->Cols; j++)
	       fprintf(Fp,"%5.15g ", ReadMat(M, i, j));
	   fprintf(Fp, "\n");
	}
//	fprintf(Fp, "\n");
}

void PrintMatrix(Matrix *M)
{
	fPrintMatrix(stdout, M);
} 

void PrintMatrixEx(Matrix *M, char *Name)
{
	int E;
	E = M->Encoding;
	
	printf("Matrix %s, Size: %d x %d, Encoding: %s\n\n",
			Name, M->Rows, M->Cols,
			(E == ME_ZERO) ? "Zero" :
			(E == ME_DIRECT) ? "Direct" :
			(E == ME_DIAGONAL) ? "Diagonal" :
			(E == ME_SPARSE_R) ? "Sparse (Row)" :
			(E == ME_SPARSE_C) ? "Sparse (Column)" :
			"Unsupported!!");
	fPrintMatrix(stdout, M);
	printf("\n");
} 

void fPrintVector(FILE *Fp, Vector *V)
{
	int i;
	
	for(i = 0; i < V->Size; i++)
//	   fprintf(Fp, "%5.15f ", ReadVec(V, i));
	   fprintf(Fp, "%g, ", ReadVec(V, i));
	fprintf(Fp,"\n");
}

void PrintVector(Vector *V)
{
	fPrintVector(stdout, V);
}

