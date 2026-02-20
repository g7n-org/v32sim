ARCH = $(shell uname -m)
LIBS =
CFLAGS =  -Wall --std=gnu18 -funsigned-char -Wno-unused-value
CFLAGS += -Wno-pointer-sign -Wno-main -Wno-int-conversion
INC = 
CC = gcc
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
TARGET = v32sim
all: $(TARGET)

debug: CFLAGS += -DDEBUG -g
debug: DEBUG = debug
debug: $(SRC) $(OBJ) $(TARGET)

%.o: %.c
	$(CC) $(INC) $(CFLAGS) -c $< -o $@ $(LIBS)

$(TARGET): $(OBJ)
	$(CC) $(INC) $(CFLAGS)    $^ -o $@ $(LIBS)

install:
	@mkdir -p /home/$(USER)/bin
	@if [ -d "/home/$(USER)/bin/bin.$(ARCH)/" ]; then cp -av $(TARGET) /home/$(USER)/bin/bin.$(ARCH)/; else cp -av $(TARGET) /home/$(USER)/bin/; fi


# Phony target for cleaning up generated files
.PHONY: clean
clean:
	@rm -vf .*.sw[op] *.save* *~ $(TARGET) $(OBJ) core errors
