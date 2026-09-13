####################################################################################
#
# v32sim Makefile
#
# directory layout:
#   src/  C sources
#   inc/  headers
#   obj/  build intermediates (.o, .d)          [generated]
#   bin/  compiled binary                       [generated]
#
####################################################################################

ARCH    = $(shell uname -m)
OS      = $(shell uname -s)

CC      = gcc
TARGET  = v32sim

SRCDIR  = src
INCDIR  = inc
OBJDIR  = obj
BINDIR  = bin

INC     = -I$(INCDIR)

CFLAGS  = -Wall --std=gnu18 -funsigned-char -Wno-unused-value
CFLAGS += -Wno-pointer-sign -Wno-main -Wno-int-conversion
CFLAGS += -Wno-unused-but-set-variable

# GNU readline; macOS ships history inside readline, Linux wants it separate
LIBS    = -lreadline -lm
ifneq ($(OS),Darwin)
    LIBS += -lhistory
endif

####################################################################################
#
# file sets
#
####################################################################################

SRC     = $(wildcard $(SRCDIR)/*.c)
OBJ     = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRC))
DEPS    = $(OBJ:.o=.d)
BIN     = $(BINDIR)/$(TARGET)

####################################################################################
#
# targets
#
####################################################################################

all: $(BIN)

$(BIN): $(OBJ) | $(BINDIR)
	$(CC) $(CFLAGS) $(INC) $(OBJ) -o $@ $(LIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INC) -MMD -MP -c $< -o $@

$(OBJDIR) $(BINDIR):
	mkdir -p $@

# development build; object files are not rebuilt on flag changes, so run
# 'make clean' when switching between this, 'asan', and the default build
debug: CFLAGS += -DDEBUG -g
debug: $(BIN)

# sanitizer build (ASan + UBSan) for hunting memory errors
asan: CFLAGS += -g -fsanitize=address,undefined -fno-omit-frame-pointer
asan: $(BIN)

install: $(BIN)
	@if [ -d ~/bin/bin.$(ARCH) ]; then \
		echo "Installing $(TARGET) to ~/bin/bin.$(ARCH)/"; \
		install -m 755 $(BIN) ~/bin/bin.$(ARCH)/$(TARGET); \
	elif [ -d ~/bin ]; then \
		echo "Installing $(TARGET) to ~/bin/"; \
		install -m 755 $(BIN) ~/bin/$(TARGET); \
	else \
		echo "Skipping: neither ~/bin/bin.$(ARCH) nor ~/bin exist"; \
	fi

sysinstall: $(BIN)
	@if [ -d /usr/local/bin ]; then \
		echo "Installing $(TARGET) to /usr/local/bin/"; \
		install -m 755 $(BIN) /usr/local/bin/$(TARGET); \
	else \
		echo "Skipping: /usr/local/bin does not exist"; \
	fi

clean:
	@rm -vf .*.sw[op] *.save* *~ core errors
	@rm -vf $(OBJ) $(DEPS) $(BINDIR)/$(TARGET)

.PHONY: all debug asan install sysinstall clean

-include $(DEPS)

