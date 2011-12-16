SRC = bumblebeed.c bbsocket.c bbglobals.c bblogger.c bbrun.c bbswitch.c
OBJ = $(SRC:.c=.o)
OUT = bumblebeed
INCLUDES =
OPTIMIZE = -g
VERSION = `git describe --tags`
CCFLAGS = -Wall -Wextra -funsigned-char $(OPTIMIZE) -DVERSION=$(VERSION)
CC = $(CROSS)gcc
LD = $(CROSS)ld
AR = $(CROSS)ar
LIBS = -lX11
.SUFFIXES: .c
.PHONY: clean default
default: $(OUT)
.c.o:
	$(CC) $(INCLUDES) $(CCFLAGS) -c $< -o $@
$(OUT): $(OBJ)
	$(CC) -o $(OUT) $(OBJ) $(LIBS)
clean:
	rm -f $(OBJ) $(OUT)
