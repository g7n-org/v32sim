ARCH = $(shell uname -m)
#LIBS =    -lreadline -lhistory -lm
LIBS =    -lreadline -lm -lfl
CFLAGS =  -Wall --std=gnu18 -funsigned-char -Wno-unused-value
CFLAGS += -Wno-pointer-sign -Wno-main -Wno-int-conversion
CFLAGS += -Wno-unused-but-set-variable
INC = 
CC = gcc
YACC = bison
LEX = flex
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

parser.tab.c parser.tab.h: v32sim.y
	$(YACC) -d v32sim.y -t

lex.yy.c: v32sim.l parser.tab.h
	$(LEX) v32sim.l

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
