CC=emcc
CFLAGS=-Ilibpng-1.6.16 -Izlib-1.2.8 -Ijsoncpp-0.7.1/include -Ilibogg-1.3.2/include -Ilibvorbis-1.3.4/include

CXX=em++
CXXFLAGS=$(CFLAGS)

EMFLAGS=-s EXPORTED_FUNCTIONS="['_main', '_initFsDone']"
LDFLAGS=-Llibpng-1.6.16/.libs -lpng16 -Lzlib-1.2.8 -lz -Ljsoncpp-0.7.1 -ljsoncpp \
	-Llibvorbis-1.3.4/lib/.libs -lvorbisfile -lvorbis \
	-Llibogg-1.3.2/src/.libs -logg

OBJS=test.o data.o testogg.o

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
