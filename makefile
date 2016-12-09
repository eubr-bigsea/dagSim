all: libs dagSim distribGen

dagSim: dagSim.o luaInterface.o ssLib.o distributions.o rnd.o
	gcc -o dagSim dagSim.o luaInterface.o ssLib.o distributions.o rnd.o \
                 matrix/libmatrix.a lua/liblua.a simlib/libsimlib.a -lm -lpthread -Wall -O3 -g

dagSim.o: dagSim.c luaInterface.h
	gcc -c -Wall -O3 dagSim.c

ssLib.o: ssLib.c ssLib.h
	gcc -c -Wall -O3 ssLib.c

luaInterface.o: luaInterface.c luaInterface.h
	gcc -c -Wall -O3 luaInterface.c

distributions.o: distributions.c
	gcc -c -Wall -O3 distributions.c

rnd.o: rnd.c
	gcc -c -Wall -O3 rnd.c


distribGen: distribGen.o luaInterface.o distributions.o rnd.o
	gcc -o distribGen distribGen.o luaInterface.o distributions.o rnd.o \
                 matrix/libmatrix.a lua/liblua.a -lm -lpthread -Wall -O3 -g

distribGen.o: distribGen.c luaInterface.h
	gcc -c -Wall -O3 distribGen.c


libs:
	cd lua && $(MAKE) posix
	cd matrix && $(MAKE)
	cd simlib && $(MAKE)

clean:
	cd lua && $(MAKE) clean
	cd matrix && $(MAKE) clean
	cd simlib && $(MAKE) clean
	rm -f *~ *.bak *.o dagSim distribGen
