PLATFORM_SRC = src/platform.cpp src/platform_linux.cpp
SRC = $(filter-out $(PLATFORM_SRC), $(wildcard src/*.cpp))
SRC += $(wildcard deps/*.cpp)

CFILES = deps/imgui.cpp deps/imgui_draw.cpp deps/imgui_demo.cpp deps/imgui_impl_sdl_gl3.cpp deps/lodepng.cpp src/draft.cpp
EDITOR_CFILES = deps/imgui.cpp deps/imgui_draw.cpp deps/imgui_demo.cpp deps/imgui_impl_sdl_gl3.cpp deps/lodepng.cpp src/editor.cpp
PLATFORM_CFILES = src/platform.cpp
LIBS = -lfreetype -L/usr/lib64 -lSDL2 -lm -lGL -lGLU -lopenal -lsndfile -lGLEW -ldl

INCLUDE_PATHS = -Ideps -Isimple_lisp

CC = clang++
CFLAGS = -g -std=c++14 -Wall -Wextra -Wno-unused-parameter $(INCLUDE_PATHS) -DDRAFT_DEBUG
LDFLAGS = $(LIBS)

OUT = build/draft.so
EDITOR_OUT = build/editor.so
PLATFORM_OUT = build/platform

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -fPIC -shared $(CFILES) -o $@ $(LDFLAGS)

editor:
	$(CC) $(CFLAGS) -fPIC -shared $(EDITOR_CFILES) -o $(EDITOR_OUT) $(LDFLAGS)

platform: $(PLATFORM_SRC)
	$(CC) $(CFLAGS) $(PLATFORM_CFILES) -o $(PLATFORM_OUT) $(LDFLAGS)

run: platform
	@cd build && ./../$(PLATFORM_OUT)

run_editor: platform
	@cd build && ./../$(PLATFORM_OUT) ./editor.so

clean:
	@rm $(OUT) $(PLATFORM_OUT)

.PHONY: run clean
