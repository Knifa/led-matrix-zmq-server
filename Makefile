OBJECTS=src/*.o
BINARIES=matryx-server


RGB_INCDIR=rpi-rgb-led-matrix/include
RGB_LIBDIR=rpi-rgb-led-matrix/lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a


CFLAGS+=-O3
LDFLAGS+=-L$(RGB_LIBDIR) -l$(RGB_LIBRARY_NAME) \
	 -lpthread -lczmq


all : matryx-server


matryx-server : $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

%.o : %.c
	$(CC) -I$(RGB_INCDIR) -c -o $@ $<


clean:
	rm -f $(OBJECTS) $(BINARIES)


FORCE:
.PHONY: FORCE
