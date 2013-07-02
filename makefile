# indicate where the object files are to be created
CC         := g++
LINKER     := $(CC)
CFLAGS     := -O3 -fopenmp -Wall -Isrc/ -Isrc/DLA/
#CFLAGS     := -O3 -Wall
#CFLAGS	   := -g -fopenmp -Wall
#CFLAGS	   := -g -Wall
#CFLAGS	   := -pg -Wall

HEADERS :=  $(shell find src -type f -name '*.h')
SOURCES :=  $(shell find src -type f -name '*.cpp')
OBJS := $(patsubst src/%.cpp, obj/%.o, $(SOURCES))

all: dxter.x

obj/%.o: src/%.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

dxter.x: $(OBJS) $(HEADERS)
	$(LINKER) $(CFLAGS) $(OBJS) -o $@

clean:
	rm -f obj/*.o obj/DLA/*.o src/*~ src/DLA/*~ *.x *~ src/*.orig src/DLA/*.orig

open:
	emacs makefile src/*cpp src/*h src/DLA/*cpp src/DLA/*h

opencpp:
	emacs src/*cpp src/DLA/*cpp

openh:
	emacs src/*h src/DLA/*h