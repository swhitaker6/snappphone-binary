CC=../emcc
CFLAGS=-W -Wall -O3 -I.
OE=.o
OUTE=
LFLAGS=-s


OBJECTS=chacha20_simple$(OE) test$(OE)

.SUFFIXES: .c

%$(OE): %.c
	$(CC) $(CFLAGS) -o $@ -c $<

ALL: test$(OUTE)

test$(OUTE): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LFLAGS)

clean:
	rm -f test$(OUTE) $(OBJECTS)
