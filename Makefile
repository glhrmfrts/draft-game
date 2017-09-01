SRC = $(wildcard src/*.cpp)
SRC += $(wildcard deps/*.cpp)

CFILES = deps/imgui.cpp deps/imgui_draw.cpp deps/imgui_demo.cpp deps/imgui_impl_sdl_gl3.cpp deps/lodepng.cpp src/draft.cpp

ifdef SystemRoot
	LIBS = -lglew32s -lfreetype-6 -lSDL2main -lSDL2 -lopengl32 -lglu32 -lgdi32 -lopenAL32 -lsndfile-1
else
	LIBS = -lfreetype -L/usr/lib64 -lSDL2 -lm -lGL -lGLU -lopenal -lsndfile -lGLEW
endif

INCLUDE_PATHS = -Ideps -IC:/c-cpp/libs/include -IC:/c-cpp/pl/lua/src -IC:/c-cpp/LuaBridge/Source

CC = clang++
CFLAGS = -O3 -std=c++14 -Wall -Wextra -Wno-unused-parameter $(INCLUDE_PATHS) -DGLEW_STATIC -DDRAFT_DEBUG
LDFLAGS = -LC:/c-cpp/libs/lib/x64 $(LIBS)

OUT = build/draft
ifdef SystemRoot
  CC = g++
	OUT = build/draft.exe
endif

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(CFILES) -o $@ $(LDFLAGS)

run: $(OUT)
	@cd build && ./../$(OUT)

clean:
	@rm $(OUT)

.PHONY: run clean
