####################################################################################
#
# v32sim Makefile
#
# directory layout:
#   src/  C sources, plus lexer.l (flex) and parser.y (bison)
#   inc/  headers
#   obj/  build intermediates (.o, .d, and the
#         flex/bison generated lexer.c, parser.c,
#         parser.h)                             [generated]
#   bin/  compiled binary                       [generated]
#
# build requirements: a C compiler, GNU readline, flex, and bison
#
####################################################################################

ARCH    = $(shell uname -m)
OS      = $(shell uname -s)

CC      = gcc
TARGET  = v32sim

FLEX    = flex
BISON   = bison
VERSION_STRING = $(shell sed -n 's/^\#define  *VERSION  *"\(.*\)".*/\1/p' inc/version.h)

SRCDIR  = src
INCDIR  = inc
OBJDIR  = obj
BINDIR  = bin

INC     = -I$(INCDIR) -I$(OBJDIR)

CFLAGS  = -Wall --std=gnu18 -funsigned-char -Wno-unused-value
CFLAGS += -Wno-pointer-sign -Wno-main -Wno-int-conversion
CFLAGS += -Wno-unused-but-set-variable

# generated flex/bison sources are not ours to keep warning-free
GENFLAGS = -Wno-unused-function -Wno-sign-compare -Wno-unused-parameter -Wno-implicit-fallthrough

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
GENSRC  = $(OBJDIR)/lexer.c $(OBJDIR)/parser.c $(OBJDIR)/asm_lexer.c $(OBJDIR)/asm_parser.c
GENHDR  = $(OBJDIR)/parser.h $(OBJDIR)/asm_parser.h
OBJ     = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRC)) $(GENSRC:.c=.o)
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

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(GENHDR) | $(OBJDIR)
	$(CC) $(CFLAGS) $(INC) -MMD -MP -c $< -o $@

# command grammar: bison writes parser.c and parser.h, flex writes lexer.c (which
# includes parser.h for the token numbers)
$(OBJDIR)/parser.c: $(SRCDIR)/parser.y | $(OBJDIR)
	$(BISON) --defines=$(OBJDIR)/parser.h -o $@ $<

$(OBJDIR)/parser.h: $(OBJDIR)/parser.c ;

$(OBJDIR)/lexer.c: $(SRCDIR)/lexer.l $(GENHDR) | $(OBJDIR)
	$(FLEX) -o $@ $<

# live assembler: a second scanner/grammar pair, generated with the prefix 'asm'
$(OBJDIR)/asm_parser.c: $(SRCDIR)/asm_parser.y | $(OBJDIR)
	$(BISON) -p asm --defines=$(OBJDIR)/asm_parser.h -o $@ $<

$(OBJDIR)/asm_parser.h: $(OBJDIR)/asm_parser.c ;

$(OBJDIR)/asm_lexer.c: $(SRCDIR)/asm_lexer.l $(OBJDIR)/asm_parser.h | $(OBJDIR)
	$(FLEX) -P asm -o $@ $<

$(OBJDIR)/parser.o $(OBJDIR)/lexer.o $(OBJDIR)/asm_parser.o $(OBJDIR)/asm_lexer.o: $(OBJDIR)/%.o: $(OBJDIR)/%.c
	$(CC) $(CFLAGS) $(GENFLAGS) $(INC) -MMD -MP -c $< -o $@

$(OBJDIR) $(BINDIR):
	mkdir -p $@

# development build; object files are not rebuilt on flag changes, so run
# 'make clean' when switching between this, 'asan', and the default build
debug: CFLAGS += -DDEBUG -g
debug: $(BIN)

# sanitizer build (ASan + UBSan) for hunting memory errors
asan: CFLAGS += -g -fsanitize=address,undefined -fno-omit-frame-pointer
asan: $(BIN)

version:
	@echo "$(TARGET) $(VERSION_STRING)"

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

archive:
	@zip -r ../v32sim-project.zip inc/ Makefile *.md src/ tests/

clean:
	@rm -vf .*.sw[op] *.save* *~ core errors *.patch *.zip ../v32sim-project.zip
	@rm -vf $(OBJ) $(DEPS) $(GENSRC) $(GENHDR) $(BINDIR)/$(TARGET)

.PHONY: all debug asan version install sysinstall clean archive

-include $(DEPS)

