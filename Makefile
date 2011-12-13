SRC = bumblebeed.c bbsocket.c bbglobals.c bblogger.c bbrun.c
OBJ = $(SRC:.c=.o)
OUT = bumblebeed
INCLUDES =
OPTIMIZE = -g
VERSION = `git describe --tags`
CCFLAGS = -Wall -Wextra -funsigned-char $(OPTIMIZE) -DVERSION=$(VERSION)
CC = $(CROSS)gcc
LD = $(CROSS)ld
AR = $(CROSS)ar
LIBS = 
.SUFFIXES: .c
.PHONY: clean default
default: $(OUT)
.c.o:
	$(CC) $(INCLUDES) $(CCFLAGS) $(LIBS) -c $< -o $@
$(OUT): $(OBJ)
	$(CC) $(LIBS) -o $(OUT) $(OBJ)
clean:
	rm -rf $(OBJ) $(OUT) *~


