CFLAGS += -g -Wall
LIBS = -lstdc++ -lm

times : times.o
	g++ -o $@ $^ ${LIBS}

%.o : %.cpp
	g++ -c ${CFLAGS} $<

# args are (1) number of iterations, (2) bucket file name, (3) state file name
test_10: times
	./times 10000 buckets1.txt state.txt

clean:
	@ rm -f *~ *.o
