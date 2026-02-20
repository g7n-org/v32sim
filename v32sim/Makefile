LIBS =
CFLAGS = -Wall --std=gnu18 -funsigned-char -Wno-unused-value -Wno-pointer-sign -Wno-main -Wno-int-conversion
INC = 
CC = gcc
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=)
BIN = v32sim
all: $(SRC) $(OBJ) $(BIN)

debug: CFLAGS += -DDEBUG -g
debug: DEBUG = debug
debug: $(SRC) $(BIN)

.c:
	@mkdir -p ../bin
	$(CC)  $(INC) -c $< $(CFLAGS) $(LIBS)

v32sim:
	$(CC)  $(INC) -o ../bin/$@ $< $(CFLAGS) $(LIBS)

install:
	@mkdir -p /home/$(USER)/bin
	@cp -av ../bin/* /home/$(USER)/bin/

clean:
	@rm -vf .*.sw[op] *.save* *~ ../bin/* core errors
