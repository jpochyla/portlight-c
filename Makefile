SOKOL_CFLAGS = -fobjc-arc -I.
SOKOL_LDFLAGS = -framework Metal -framework Cocoa -framework MetalKit -framework Quartz -framework AudioToolbox
SOKOL_SHDC = vendor/sokol-tools-bin/bin/osx/sokol-shdc --slang metal_macos

LDFLAGS = -lChipmunk
CFLAGS = -g -Wall -Wextra -std=c11 \
	-I/usr/local/include -Isrc -Ibuild -I. \
	--system-header-prefix=vendor

SRC = src/main.c build/sokol.o build/microui.o

run: build/game
	build/game

build/game: $(SRC) src/gfx.h src/phx.h src/ui.h src/utils.h build/shd_trace.h build/shd_screen.h
	$(CC) -o $@ $(SRC) $(CFLAGS) $(LDFLAGS) $(SOKOL_LDFLAGS)

build/shd_trace.h: src/shd_trace.glsl
	$(SOKOL_SHDC) --input $< --output $@

build/shd_screen.h: src/shd_screen.glsl
	$(SOKOL_SHDC) --input $< --output $@

build/sokol.o: src/sokol.m
	$(CC) -c -o $@ $< $(SOKOL_CFLAGS)

build/microui.o: vendor/microui/src/microui.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f build/game build/shd_trace.h build/shd_screen.h build/sokol.o build/microui.o
