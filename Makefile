CC=emcc
CFLAGS=-Ilibpng-1.6.16 -Izlib-1.2.8

CXX=em++
CXXFLAGS=$(CFLAGS)

EMFLAGS=-s EXPORTED_FUNCTIONS="['_main', '_initFsDone']"
LDFLAGS=-Llibpng-1.6.16/.libs -lpng16 -Lzlib-1.2.8 -lz

OBJS=test.o data.o

PROGRAM=program.js
PROGHTM=program.html

.PHONY: all clean html

all: $(PROGRAM)

html: $(PROGHTM)


$(PROGRAM) $(PROGHTM): $(OBJS)
	$(CXX) $(EMFLAGS) -o $@ $(OBJS) $(LDFLAGS) --emrun


clean:
	rm -f $(PROGRAM)
	rm -f $(PROGHTM)
	rm -f $(OBJS)
