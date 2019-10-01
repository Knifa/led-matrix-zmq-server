BIN_DIR=bin

BIN=$(BIN_DIR)/matryx-server
SRC = $(wildcard src/*.cpp)
OBJ = $(patsubst src/%.cpp, bin/%.o, $(SRC))


RGB_INCDIR=rpi-rgb-led-matrix/include
RGB_LIBDIR=rpi-rgb-led-matrix/lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a

CFLAGS+=-O3 -Wall
LDFLAGS+=-L$(RGB_LIBDIR) -l$(RGB_LIBRARY_NAME) \
	 -lpthread -lczmq

all : $(BIN)

$(BIN) : $(OBJ)
	$(CXX) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS)

$(BIN_DIR)/%.o : src/%.cpp | $(BIN_DIR)
	$(CXX) $(CFLAGS) -I$(RGB_INCDIR) -c -o $@ $<

$(BIN_DIR):
	mkdir $@

clean:
	rm -f $(OBJ) $(BIN)

FORCE:
.PHONY: FORCE
