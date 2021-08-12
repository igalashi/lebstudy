#
#
#

CC = gcc
CFLAGS = -Wall -g -O
INCLUDES =
LIBS =

all: udpreg

udpreg: udpreg.c
	$(CC) $(CFLAGS) -o $@ $(INCLUDES) \
		udpreg.c \
		$(LIBS)

clean:
	rm -f udpreg
