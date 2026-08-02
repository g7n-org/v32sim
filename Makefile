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

install: $(TARGET)
	@if [ -d ~/bin/bin.$(ARCH) ]; then \
		echo "Installing $(TARGET) to ~/bin/bin.$(ARCH)/"; \
		install -m 755 $(TARGET) ~/bin/bin.$(ARCH)/$(TARGET); \
	elif [ -d ~/bin ]; then \
		echo "Installing $(TARGET) to ~/bin/"; \
		install -m 755 $(TARGET) ~/bin/$(TARGET); \
	else \
		@echo "Skipping: neither ~/bin/bin.$(ARCH) nor ~/bin exist"; \
	fi

sysinstall: $(TARGET)
	@if [ -d /usr/local/bin ]; then \
		echo "Installing $(TARGET) to /usr/local/bin/"; \
		install -m 755 $(TARGET) /usr/local/bin/$(TARGET); \
	else \
		@echo "Skipping: /usr/local/bin does not exist"; \
	fi

# Phony target for cleaning up generated files
.PHONY: clean
clean:
	@rm -vf .*.sw[op] *.save* *~ $(TARGET) $(OBJ) core errors
