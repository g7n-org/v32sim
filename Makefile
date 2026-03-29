ARCH = $(shell uname -m)
#LIBS =    -lreadline -lhistory -lm
LIBS =    -lreadline -lm
CFLAGS =  -Wall --std=gnu18 -funsigned-char -Wno-unused-value
CFLAGS += -Wno-pointer-sign -Wno-main -Wno-int-conversion
CFLAGS += -Wno-unused-but-set-variable
INC = 
CC = gcc
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
TARGET = v32sim
OS = $(shell uname -s)

# Conditional check for (NOT) macOS -- handle gnu readline history library
ifneq ($(OS),Darwin)
    LIBS += -lhistory 
endif

all: $(TARGET)

debug: CFLAGS += -DDEBUG -g
debug: DEBUG = debug
debug: $(SRC) $(OBJ) $(TARGET)

%.o: %.c
	$(CC) $(INC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	$(CC) $(INC) $(CFLAGS)    $^ -o $@ $(LIBS)

install:
	@mkdir -p /home/$(USER)/bin
	@if [ -d "/home/$(USER)/bin/bin.$(ARCH)/" ]; then cp -av $(TARGET) /home/$(USER)/bin/bin.$(ARCH)/; else cp -av $(TARGET) /home/$(USER)/bin/; fi

sysinstall:
	@cp -av $(TARGET) /usr/local/bin/$(TARGET)
	@chmod 0755 /usr/local/bin/$(TARGET)

# Phony target for cleaning up generated files
.PHONY: clean
clean:
	@rm -vf .*.sw[op] *.save* *~ $(TARGET) $(OBJ) core errors
