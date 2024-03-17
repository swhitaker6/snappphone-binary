COMPLR = $(shell which clang >/dev/null; echo $$?)
ifeq "$(COMPLR)" "0"
CC = ./emcc
else
rmcpCC = ./emcc
endif

#CC = clang
#CC = gcc


ifeq ($(CC), clang)
    CFLAGS = -O2 -s -pedantic -Wall -Wextra -Weverything 
else
    # CFLAGS = -O2 -s "EXPORTED_FUNCTIONS=['_main', '_genTexture', '_getTexture']" -s "EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap', 'getValue']" -s ALLOW_MEMORY_GROWTH=0 -s TOTAL_MEMORY=1024MB --pre-js pre-demo.js --post-js post-demo.js -pedantic -Wall -Wextra -std=c99
    # CFLAGS = -O2 -s "EXPORTED_FUNCTIONS=['_main', '_genTexture', '_getTexture']" -s "EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap', 'getValue']" -s ALLOW_MEMORY_GROWTH=0 -s TOTAL_MEMORY=1024MB --preload-file images/stride.png --pre-js pre-demo.js --post-js post-demo.js -pedantic -Wall -Wextra -std=c99
    # CFLAGS = -O2 -s "EXPORTED_FUNCTIONS=['_main', '_genTexture', '_getTexture']" -s LLD_REPORT_UNDEFINED -s "EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap', 'getValue']" -s ALLOW_MEMORY_GROWTH --preload-file images/sand.png --pre-js pre-demo.js --post-js post-demo.js -pedantic -Wall -Wextra -std=c99 -g
    # CFLAGS = -O2 -s "EXPORTED_FUNCTIONS=['_main', '_genTexture', '_getTexture']" -s LLD_REPORT_UNDEFINED -s "EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap', 'getValue']"  -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS=[png] -s ALLOW_MEMORY_GROWTH --preload-file images/sand.png --pre-js pre-demo.js --post-js post-demo.js -pedantic -Wall -Wextra -std=c99 -g
endif

# CFLAGS = -O2 -s "EXPORTED_FUNCTIONS=['_main', '_genTexture', '_getTexture']" -s "EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap', 'getValue']" -s ALLOW_MEMORY_GROWTH  --pre-js pre-demo.js --post-js post-demo.js -pedantic -Wall -Wextra -std=c99

# CFLAGS = -O2 -s "EXPORTED_FUNCTIONS=['_main', '_genTexture', '_getTexture']" -s "EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap', 'getValue']" -s ALLOW_MEMORY_GROWTH -s USE_SDL=2 -s USE_SDL_IMAGE=2 --preload-file images/sand.png
# CFLAGS = -O2 -s "EXPORTED_FUNCTIONS=['_main', '_genTexture', '_getTexture']" -s "EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap', 'getValue']" -s ALLOW_MEMORY_GROWTH -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS=[png]
CFLAGS = -O2 -s "EXPORTED_FUNCTIONS=['_main', '_genTexture', '_getTexture']" -s "EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap', 'getValue']" -s ALLOW_MEMORY_GROWTH --pre-js pre-demo.js -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS=[png] -s ASSERTIONS=1
# CFLAGS = -O2 -s "EXPORTED_FUNCTIONS=['_main', '_genTexture', '_getTexture']" -s "EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap', 'getValue']" -s ALLOW_MEMORY_GROWTH --pre-js pre-demo.js -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS=[png] --preload-file images/motorcycle.png
# CFLAGS = -O2 -s ERROR_ON_UNDEFINED_SYMBOLS=0 -s "EXPORTED_FUNCTIONS=['_main', '_genTexture', '_getTexture']" -s "EXPORTED_RUNTIME_METHODS=['ccall', 'cwrap', 'getValue']" -s ALLOW_MEMORY_GROWTH -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS=[png] --preload-file images/sand.png -I/home/swhitaker6/project/ImageMagick-7.1.0

snapp_binary_master.html: snapp_binary_master.o chacha20_simple.o 


	$(CC) $(CFLAGS) $^ -o $@

chacha20_simple.o: chacha20_simple.c chacha20_simple.h
	$(CC) $(CFLAGS) -c -o $@ $<

snapp_binary_master.o: snapp_binary_master.c

	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean verify

clean:
	@echo cleaning...


	rm -f snapp_binary_master.o snapp_binary_master.js snapp_binary_master.html snapp_binary_master.wasm snapp_binary_master.data


verify:
	@cbmc demo.c chacha20_simple.c -D BACK_TO_TABLES --partial-loops --bounds-check --pointer-check --div-by-zero-check --memory-leak-check --signed-overflow-check --unsigned-overflow-check --refine

