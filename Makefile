OBJECTS=matrix-server.o
BINARIES=matrix-server

RGB_INCDIR=matrix/include
RGB_LIBDIR=matrix/lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a

LDFLAGS+=-L$(RGB_LIBDIR) -l$(RGB_LIBRARY_NAME) \
	 -lpthread -lczmq

all : matrix-server

run : matrix-server
	sudo ./matrix-server

matrix-server : $(OBJECTS) $(RGB_LIBRARY)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(RGB_LIBRARY): FORCE
	$(MAKE) -C $(RGB_LIBDIR)

matrix-server.o : matrix-server.c

%.o : %.c
	$(CXX) -I$(RGB_INCDIR) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(BINARIES)
#	$(MAKE) -C $(RGB_LIBDIR) clean

FORCE:
.PHONY: FORCE
