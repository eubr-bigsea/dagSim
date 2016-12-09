all: libsimlib.a

libsimlib.a: CalendarEvent.o CalendarQueue.o heap.o list.o utilities.o
	ar rcu libsimlib.a CalendarEvent.o CalendarQueue.o heap.o list.o utilities.o
	ranlib libsimlib.a

CalendarEvent.o: CalendarEvent.c CalendarEvent.h
	gcc -c -Wall -O3 CalendarEvent.c

CalendarQueue.o: CalendarQueue.c CalendarQueue.h
	gcc -c -Wall -O3 CalendarQueue.c

heap.o: heap.c heap.h
	gcc -c -Wall -O3 heap.c

list.o: list.c list.h auxLists.h
	gcc -c -Wall -O3 list.c

utilities.o: utilities.c utilities.h
	gcc -c -Wall -O3 utilities.c



clean:
	rm -f *~ *.bak *.a *.o
