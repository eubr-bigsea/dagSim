#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "Matrix.h"





#define MAX_EXP_ITER 20000
#define MIN_EXP_VAL  10e-10

void MatExpAndIntN(Matrix *Q, double t, Matrix **Exp, Matrix **Int)
{
	Matrix *T1, *T2, *Acc, *An, *A, *Acci;
	int i,j;
	double Xnk, SXnk;
	double ql, qa, aql;
	
	A = CreateMatrix(Q->Rows, Q->Cols);
	An = CreateIdentityMatrix(Q->Rows);
	Acc = CreateMatrix(Q->Rows, Q->Cols);
	Acci = CreateMatrix(Q->Rows, Q->Cols);
				
	aql = 0.0;
	ql = 0.0;
	
	for(i = 0; i < Q->Rows; i++)
	   for(j = 0; j < Q->Cols; j++) {
	      qa = ReadMat(Q, i, j);
	      qa = (qa > 0.0) ? qa : -qa;
	      if(qa > aql) {ql = aql = qa; /*ql = ReadMat(Q, i, j);*/}
	   }

	for(i = 0; i < Q->Rows; i++)
	   for(j = 0; j < Q->Cols; j++) {
	      WriteMat(A, i, j, ((i == j) ? 1.0 : 0.0) +
	      		ReadMat(Q, i, j) / ql);
	   }
	   	
	for(i = 0; i < Q->Rows; i++) {
		WriteMat(Acc, i, i, 1.0);
		WriteMat(Acci, i, i, (1.0 - exp(-ql*t))/ql);
	}

	Xnk = 1.0;
	SXnk = (1.0 - exp(-ql*t));

	for(i = 1; i < MAX_EXP_ITER; i++) {
		Xnk = Xnk * t * ql / (double)i;
		SXnk -= Xnk * exp(-ql*t);
		
		if(SXnk < MIN_EXP_VAL) break;
		
//printf("%f ",  (1.0 - SXnk * exp(-ql*t))/ql);
		
		T1 = MultMatMatN(An, A);
		DestroyMatrix(An);
		An = T1;
		
		T1 = MultMatScalN(An, Xnk);
		T2 = AddMatN(Acc,T1);
		DestroyMatrix(T1);
		DestroyMatrix(Acc);
		Acc = T2;

		T1 = MultMatScalN(An, SXnk /ql);
//printf("\n>>>>>>%g \n",  SXnk/ql);
//PrintMatrix(An);
//PrintMatrix(T1);
		T2 = AddMatN(Acci,T1);
		DestroyMatrix(T1);
		DestroyMatrix(Acci);
		Acci = T2;
	}
//if(getchar()==27)exit(0);
//printf("\n Iter = %d \n", i);	
	DestroyMatrix(An);
	DestroyMatrix(A);
	
	for(i = 0; i < Q->Rows; i++)
	   for(j = 0; j < Q->Cols; j++) {
	      WriteMat(Acc, i, j,
	      		ReadMat(Acc, i, j) * exp(-ql*t));
	   }
	
	*Int = Acci;
	*Exp = Acc;
}

#define ERR_P_F        64
#define TAY_EXP_END    48
#define TAY_EXP_ERROR  10e-15

void MatExpAndInt2N(Matrix *Q, double t, Matrix **Exp, Matrix **Int)
{
	Matrix *A, *dQ, *T1, *T2, *Ait, *C;
	
	double DeltaT, Max, Val;
	int i,j,k, Nmax, Niters=TAY_EXP_END-1;
	
	if(t <= 0) {
	  A = CreateMatrix(Q->Rows, Q->Cols);
	  Ait = CreateMatrix(Q->Rows, Q->Cols);
	  for(i = 0; i < Q->Rows; i++)
	    WriteMat(A, i, i, 1.0);
	  *Int = Ait;
	  *Exp = A;
	  return;
	}
	
	Max = 0.0;
	for(i = 0; i < Q->Rows; i++)
	  for(j = 0; j < Q->Cols; j++) {
	  	Val = ReadMat(Q, i, j);
	  	Val = (Val < 0.0) ? -Val : Val;
	  	if(Val > Max) Max = Val;
	  }
	
	Nmax = (int)(log(Q->Rows * Max * ERR_P_F * t)/log(2)) + 1;
	DeltaT = t / pow(2, Nmax);
	
//***printf(" n = %d Dt = %g Max=%g\n", Nmax, DeltaT, Max);

	/* Compute dQ = exp(Q * DeltaT) */
	
	dQ = CreateMatrix(Q->Rows, Q->Cols);
	T1 = CreateMatrix(Q->Rows, Q->Cols);
	C = CreateMatrix(Q->Rows, Q->Cols);
	
	for(i = 0; i < Q->Rows; i++) {
//Ext1	  WriteMat(dQ, i, i, 1.0);
	  WriteMat(T1, i, i, 1.0);
	}
	
	for(i = 1; i < TAY_EXP_END; i++) {
	  T2 = MultMatMatN(T1, Q);
	  DestroyMatrix(T1);
	  T1 = MultMatScalN(T2, DeltaT / (double)i);
	  DestroyMatrix(T2);
	  
	  T2 = AddMatN(dQ, T1);
	  DestroyMatrix(dQ);
	  dQ = T2;
	  {
	    double MV = 0.0, CV, Qll;
	    for(j = 0; j < T1->Rows; j++)
	      for(k = 0; k < T1->Cols; k++) {
	        Qll = ReadMat(dQ, j, k);
	        CV = ReadMat(T1, j, k) / (Qll != 0.0 ? Qll : 1.0);
	        CV = (CV > 0.0) ? CV : -CV;
	        if(CV > MV) MV = CV;
	      }
//***            printf("\n} %d --> %g }",i,MV);
	    if(MV < TAY_EXP_ERROR) {
	       Niters = i;
	       break;
	    }
	  }
	} 
//PrintMatrixEx(dQ, "Delta Q");
	/* Compute C = ..... */

	for(i = 0; i < Q->Rows; i++) {
	  WriteMat(C, i, i, DeltaT);
	  WriteMat(T1, i, i, DeltaT);
	}
	
	for(i = 1; i < Niters; i++) {
	  T2 = MultMatMatN(T1, Q);
	  DestroyMatrix(T1);
	  T1 = MultMatScalN(T2, DeltaT / (double)(i+1));
	  DestroyMatrix(T2);
	  
	  T2 = AddMatN(C, T1);
	  DestroyMatrix(C);
	  C = T2;
	  {
	    double MV = 0.0, CV, Qll;
	    for(j = 0; j < T1->Rows; j++)
	      for(k = 0; k < T1->Cols; k++) {
	        Qll = ReadMat(C, j, k);
	        CV = ReadMat(T1, j, k) / (Qll != 0.0 ? Qll : 1.0);
	        CV = (CV > 0.0) ? CV : -CV;
	        if(CV > MV) MV = CV;
	      }
//***	    printf("\n}] %d --> %g }]",i,MV);
//	    if(MV < TAY_EXP_ERROR) {
//	       break;
//	    }
	  }
	} 

	/* Compute exp(Q * t) = dQ ^ n */
	/* Compute int_0^x exp(Q * t) */

	A = DupMatrix(dQ);
//	Ait = C;
	Ait = CreateMatrix(C->Rows, C->Cols);

	for(i = 0; i < Nmax; i++) {
	  T2 = MultMatMatN(Ait, A);
	  T1 = AddMatN(T2, A);
	  DestroyMatrix(T2);
	  T2 = MultMatScalN(T1, 0.5);
	  DestroyMatrix(T1);
/*Ext1*/  T1 = AddMatN(T2, Ait);
/*Ext1*/  DestroyMatrix(T2);
	  DestroyMatrix(Ait);
/*Ext1*/  Ait = T1;
	  
	  T1 = MultMatMatN(A, A);
/*Ext1*/  T2 = AddMatN(T1, A);
/*Ext1*/  DestroyMatrix(T1);
/*Ext1*/  T1 = AddMatN(T2, A);
/*Ext1*/  DestroyMatrix(T2);
	  DestroyMatrix(A);
	  A = T1;
/**/{
int ui,uj;double uMax,uMin,uEl;uMax = 0.0; uMin = 0.0;
for(ui = 0; ui < A->Rows; ui++){for(uj = 0; uj < A->Cols; uj++){
uEl=ReadMat(A,ui,uj);/**printf("%g ", uEl);**/
if(uEl>uMax)uMax=uEl; if(uEl<uMin)uMin=uEl;}/**printf("\n");**/}
//****printf("Iter %d >%g <%g",i,uMax,uMin);fflush(stdout);
//if(getchar() == 27) exit(0);
}/**/
	}
	
	for(i = 0; i < Q->Rows; i++)
		WriteMat(A, i, i, ReadMat(A, i, i) + 1.0);

	for(i = 0; i < Q->Rows; i++)
		WriteMat(Ait, i, i, ReadMat(Ait, i, i) + 1.0);
	T2 = MultMatMatN(C, Ait);
	DestroyMatrix(Ait);
	Ait = MultMatScalN(T2, pow(2, Nmax));

	*Int = Ait;
	*Exp = A;
}


#define MAX_UNIFORMIZATION_ITER 1000000
#define UNIFORMIZATION_PREC 1.0e-12

Matrix **MultVecMatExpTsN(Matrix *P0, Matrix *Q, Vector *T)
{
	Matrix **out, *Tmp1, *C, *Tmp2;
//	Vector *Factor, *ExpLambda *SFact;
	int i,j, NTs;
	double Lambda, Val;


	double **Coeff;		// Per risolvere il problema dei coefficienti
	int *NCoeff;
	int *CoeffStart;
	double *LambdaT;

	
	if((P0->Cols != Q->Rows) || (Q->Rows != Q->Cols)) {
		printf("\nError! (P0 e^Qt) wrong Matrix/Vector Size\n\n");
		return NULL;
	}
	
	Lambda = -ReadMat(Q, 0, 0);		// Calcola Lambda per la randomizazione
	for(i = 1; i < Q->Rows; i++) {
	    Val = -ReadMat(Q, i, i);
	    if(Val > Lambda)
	    Lambda = Val;
	}
	C = MultMatScalN(Q, 1.0 / Lambda);	// Trova la matrice DTMC equicalente
	for(i = 0; i < Q->Rows; i++) {
	    Val = ReadMat(C, i, i) + 1.0;
	    WriteMat(C, i, i, Val);
	}
//printf("Lambda = %lg\n", Lambda);
//PrintMatrix(C);

/*	Factor = CreateVector(T->Size);
	ExpLambda = CreateVector(T->Size);
	SFact = CreateVector(T->Size);*/
	out = calloc(T->Size, sizeof(Matrix *));
	
	Coeff = (double **)calloc(T->Size, sizeof(double *));
	NCoeff = (int *)calloc(T->Size, sizeof(int));
	CoeffStart = (int *)calloc(T->Size, sizeof(int));
	LambdaT = (double *)calloc(T->Size, sizeof(double));
	
	NTs = 0;
	for(i = 0; i < T->Size; i++) {		// Crea i fattori
//		double ExpL;
//		double LambdaT;
		
		LambdaT[i] = Lambda * ReadVec(T, i);
		NCoeff[i] = floor(LambdaT[i]);
		Coeff[i] = (double *)calloc(NCoeff[i]+1, sizeof(double));
		Coeff[i][NCoeff[i]] = 1.0;
//printf("Lambda T = %g\n", LambdaT[i]);
//printf("NCoeff[%d] = %d\n", i, NCoeff[i]);
		for(j = NCoeff[i]-1; j >= 0; j--) {
			double Fmv;
			
			Fmv = (double)(j+1) / LambdaT[i];
			Coeff[i][j] = Coeff[i][j+1] * Fmv;
//printf("C[%d][%d] = %g\n", i, j, Coeff[i][j]);
			if(Coeff[i][j] < UNIFORMIZATION_PREC)
				break;
		}
		CoeffStart[i] = j;
//printf("CoeffStart[%d] = %d\n", i, CoeffStart[i]);
		
/*		ExpL =  exp(-Lambda * ReadVec(T, i));
		WriteVec(ExpLambda, i, ExpL);
		WriteVec(Factor, i, ExpL);
		WriteVec(SFact, i, ExpL);*/

//		out[i] = DupMatrix(P0);
		out[i] = CreateMatrixEnc(P0->Rows, P0->Cols, ME_SPARSE_R_DIRECT_ROWS);
//		if(CoeffStart[i] == 0) {
//			MultMatScal(out[i], P0, Coeff[i][0]);
//		}	
		if(ReadVec(T, i) > 0.0) NTs++;
	}
	
//printf("\n\n Lambda = %g \n\n", Lambda);
//PrintMatrixEx(Q, "Q");
//PrintMatrixEx(C, "C");

	
	Tmp2 = DupMatrix(P0);
	for(i = 0; i < MAX_UNIFORMIZATION_ITER; i++) {
//printf("%ld >%d\n",time(NULL),i);		
		for(j = 0; j < T->Size; j++) {
		    double SF;
		    
		    if((i >=  CoeffStart[j]) && (i <= NCoeff[j])) {
		    	SF = Coeff[j][i];
		    } else if(i > NCoeff[j]) {
		    	SF = Coeff[j][NCoeff[j]] * LambdaT[j] / (double)i;
//printf("DISCENDENTE: %d %g * %g = %g\n", j,Coeff[j][NCoeff[j]], LambdaT[j], SF);
		    	Coeff[j][NCoeff[j]] = SF;
		    } else
		        SF = 0.0;
		    
		    if(SF > 0.0) {
			if(SF < UNIFORMIZATION_PREC) {
				if(i > NCoeff[j]) {
			    		Coeff[j][NCoeff[j]] = 0.0;
					NTs--;
				}
			} else {
			    MultSumMatScal(out[j], Tmp2, SF);
//printf("T: %d, CS: %d, NC: %d, i: %d, C=%f\n", j, CoeffStart[j], NCoeff[j], i, SF); 
			}
		    }
		}
		Tmp1 = CreateMatrixEnc(P0->Rows, P0->Cols, ME_SPARSE_R_DIRECT_ROWS);
		MultMatMat(Tmp1, Tmp2, C);
//PrintMatrix(Tmp1);
//printf("%ld <%d\n",time(NULL),i);
		DestroyMatrix(Tmp2);
		
		Tmp2 = Tmp1;
		if(NTs <= 0) break;
	}
	
//printf("\n\n Uniformization iters: %d\n\n", i);
	DestroyMatrix(Tmp1);

	for(i = 0; i < T->Size; i++) free(Coeff[i]);
	free(Coeff);
	free(LambdaT);
	free(NCoeff);
	free(CoeffStart);
	
//	DestroyVector(Factor);
//	DestroyVector(SFact);
//	DestroyVector(ExpLambda);
	DestroyMatrix(C);

	for(j = 0; j < T->Size; j++) {
	    for(i = 0; i < out[j]->Rows; i++) {
	    	int k;
	    	double Norm = 0.0;
	    	
	    	for(k = 0; k < out[j]->Cols; k++)
	    	    Norm += ReadMat(out[j], i, k);
	    	    
	    	for(k = 0; k < out[j]->Cols; k++)
	    	    WriteMat(out[j], i, k, ReadMat(out[j], i, k) / Norm);
	    }

//printf("\n\n T = %f;", ReadVec(T, j));
//PrintMatrixEx(out[j], "Sol=");
	}
	
	return out;
}




/**************************************
void ExpTry()
{
	Matrix *Q, *Exp, *Int, *Exp2, *Int2;
	double X;
	int i,j;
	
	Q = CreateMatrix(N_STATES, N_STATES);
	for(i = 0; i < N_STATES; i++)
	   for(j = 0; j < N_STATES; j++)
	      WriteMat(Q, i, j, dQ[i][j]);
	
	for(X = 0.0; X < UP_B + DELTA_X/2; X += DELTA_X) {

		MatExpAndIntN(Q, X, &Exp, &Int);
		printf("---------------------------\n X = %f : \n\n",X);
		PrintMatrix(Exp);
		PrintMatrix(Int);

		MatExpAndInt2N(Q, X, &Exp2, &Int2);
		PrintMatrix(Exp2);
		PrintMatrix(Int2);
		

		DestroyMatrix(Exp);
		DestroyMatrix(Int);
		DestroyMatrix(Exp2);
		DestroyMatrix(Int2);
	}


	DestroyMatrix(Q);

}
***************************************/

/*********
double Pk(double X)
{
	if((X >= -1.0) && (X <= 1.0))
		return X;
	else if(X > 1.0)
		return (log(X) + 1.0);
	else
		return -(log(-X) + 1.0);
}

*************/
