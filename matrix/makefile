all: libmatrix.a

libmatrix.a: Matrix.o MatrixOp.o MatrixSys.o MatrixManip.o MatrixExp.o
	ar rcu libmatrix.a Matrix.o MatrixOp.o MatrixSys.o MatrixManip.o MatrixExp.o
	ranlib libmatrix.a

Matrix.o: Matrix.c Matrix.h
	gcc -c -Wall -O3 Matrix.c

MatrixOp.o: MatrixOp.c Matrix.h
	gcc -c -Wall -O3 MatrixOp.c

MatrixSys.o: MatrixSys.c Matrix.h
	gcc -c -Wall -O3 MatrixSys.c

MatrixManip.o: MatrixManip.c Matrix.h
	gcc -c -Wall -O3 MatrixManip.c

MatrixExp.o: MatrixExp.c Matrix.h
	gcc -c -Wall -O3 MatrixExp.c


clean:
	rm -f *~ *.bak *.a *.o
