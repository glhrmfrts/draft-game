SRC = $(wildcard src/*.cpp)
SRC += $(wildcard deps/*.cpp)

ifdef SystemRoot
	LIBS = -lglew32s -lfreetype-6 -lSDL2main -lSDL2 -lopengl32 -lglu32 -lgdi32 -lopenAL32 -lsndfile-1
else
	LIBS = -lfreetype -L/usr/lib64 -lSDL2 -lm -lGL -lGLU -lopenal -lsndfile -lGLEW
endif

INCLUDE_PATHS = -Ideps -IC:/c-cpp/libs/include -IC:/c-cpp/pl/lua/src -IC:/c-cpp/LuaBridge/Source

CC = clang++
CFLAGS = -g -std=c++14 -Wall -Wextra -Wno-unused-parameter $(INCLUDE_PATHS) -DGLEW_STATIC -DDRAFT_DEBUG
LDFLAGS = -LC:/c-cpp/libs/lib/x64 $(LIBS)

OUT = build/draft
ifdef SystemRoot
	OUT = build/draft.exe
endif

$(OUT): $(SRC)
	$(CC) $(CFLAGS) deps/lodepng.cpp src/draft.cpp -o $@ $(LDFLAGS)

run: $(OUT)
	@cd build && ./../$(OUT)

clean:
	@rm $(OUT)

.PHONY: run clean
