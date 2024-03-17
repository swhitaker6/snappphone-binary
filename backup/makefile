COMPLR = $(shell which clang >/dev/null; echo $$?)
ifeq "$(COMPLR)" "0"
CC = emcc
else
CC = emcc
endif

#CC = clang
#CC = gcc


ifeq ($(CC), clang)
    CFLAGS = -O2 -s -pedantic -Wall -Wextra -Weverything 
else
    CFLAGS = -O2 -s "EXPORTED_FUNCTIONS=['_main', '_genTexture', '_getTexture']" -s "EXTRA_EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap', 'getValue']"  -Izlib -Ilibpng12 --pre-js pre-demo.js --post-js post-demo.js  -pedantic -Wall -Wextra -std=c99 -g
endif

demo.html: demo.o aes128.o libpng.a libz.a
	$(CC) $(CFLAGS) $^ -o $@

aes128.o: aes128.c aes128.h
	$(CC) $(CFLAGS) -c -o $@ $<

demo.o: demo.c demo.h
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean verify

clean:
	@echo cleaning...
	rm -f demo *.o

verify:
	@cbmc demo.c aes128.c -D BACK_TO_TABLES --partial-loops --bounds-check --pointer-check --div-by-zero-check --memory-leak-check --signed-overflow-check --unsigned-overflow-check --refine
	# cbmc demo.c aes128.c --function gf_alog --unwind 288 --bounds-check --pointer-check --div-by-zero-check --memory-leak-check --signed-overflow-check --unsigned-overflow-check --refine
	# cbmc demo.c aes128.c --function gf_log --bounds-check --pointer-check --div-by-zero-check --memory-leak-check --signed-overflow-check --unsigned-overflow-check --refine
