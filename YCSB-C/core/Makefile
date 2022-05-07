CC=g++
SRC=$(wildcard *.cc)
OBJ=$(SRC:.cc=.o)

all: $(OBJ)

%.o: %.cc
	$(CC) $(CFLAGS) -I../ -c $< -o $@

clean:
	$(RM) *.o *.a
