COMPLR = $(shell which clang >/dev/null; echo $$?)
ifeq "$(COMPLR)" "0"
CC = ./emcc
else
CC = ./emcc
endif

#CC = clang
#CC = gcc


ifeq ($(CC), clang)
    CFLAGS = -O2 -s -pedantic -Wall -Wextra -Weverything 
else
    CFLAGS = -O2 -s "EXPORTED_FUNCTIONS=['_main', '_genTexture', '_getTexture']" -s LLD_REPORT_UNDEFINED -s "EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap', 'getValue']" -s ALLOW_MEMORY_GROWTH --pre-js pre-demo.js --post-js post-demo.js
    #CFLAGS = -O2 -s "EXPORTED_FUNCTIONS=['_main', '_genTexture', '_getTexture']" -s LLD_REPORT_UNDEFINED -s "EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap', 'getValue']"  -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS=[png] -s ALLOW_MEMORY_GROWTH --preload-file images/sand.png --pre-js pre-demo.js --post-js post-demo.js
endif

demo.html: demo.o chacha20_simple.o libpng.a libz.a 
#demo_test_reference.html: demo_test_reference.o chacha20_simple.o libpng.a libz.a 
	$(CC) $(CFLAGS) $^ -o $@

chacha20_simple.o: chacha20_simple.c chacha20_simple.h
	$(CC) $(CFLAGS) -c -o $@ $<

demo.o: demo.c demo.h
#demo_test_reference.o: demo_test_reference.c demo.h
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean verify

clean:
	@echo cleaning...
	rm -f demo.o demo.js demo.html demo.wasm chacha20_simple.o 
	#rm -f demo_test_reference.o demo_test_reference.js demo_test_reference.html demo_test_reference.wasm chacha20_simple.o

verify:
	@cbmc demo.c chacha20_simple.c -D BACK_TO_TABLES --partial-loops --bounds-check --pointer-check --div-by-zero-check --memory-leak-check --signed-overflow-check --unsigned-overflow-check --refine
	# cbmc demo.c aes128.c -D BACK_TO_TABLES --partial-loops --bounds-check --pointer-check --div-by-zero-check --memory-leak-check --signed-overflow-check --unsigned-overflow-check --refine
	# cbmc demo.c aes128.c --function gf_alog --unwind 288 --bounds-check --pointer-check --div-by-zero-check --memory-leak-check --signed-overflow-check --unsigned-overflow-check --refine
	# cbmc demo.c aes128.c --function gf_log --bounds-check --pointer-check --div-by-zero-check --memory-leak-check --signed-overflow-check --unsigned-overflow-check --refine
