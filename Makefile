CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -fPIC -fvisibility=hidden -Wall -Wextra -Wno-unused-parameter -Isrc -U_GNU_SOURCE -D_DEFAULT_SOURCE -MMD -MP
LDFLAGS ?= -shared -pthread -ldl -static-libstdc++ -static-libgcc

SRC = src/main.cpp src/status.cpp src/player_store.cpp src/admin_store.cpp src/recipe_settings_store.cpp src/engine/engine_core.cpp src/engine/uobject.cpp \
      src/engine/players.cpp src/engine/kills.cpp src/engine/resources.cpp src/engine/npcs.cpp src/engine/admin_teleport.cpp src/utils/memory.cpp src/utils/glibc_compat.cpp
SRC += src/engine/admin_storage.cpp
SRC += src/engine/admin_gravestone.cpp
SRC += src/recipe_snapshot_store.cpp src/engine/player_recipes.cpp src/quest_snapshot_store.cpp src/engine/player_quests.cpp
OBJ = $(SRC:.cpp=.o)
DEP = $(OBJ:.o=.d)
OUT = dist/libdwstatus.so

all: $(OUT) dist/zanik-compat-analyzer

dist/zanik-compat-analyzer: tools/compat_analyzer.cpp
	mkdir -p dist
	$(CXX) -std=c++17 -O2 $< -o $@ -static

$(OUT): $(OBJ)
	mkdir -p dist
	$(CXX) $(OBJ) -o $@ $(LDFLAGS) -Wl,--no-undefined

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(DEP) $(OUT)

-include $(DEP)

test: $(OBJ)
	$(CXX) $(CXXFLAGS) tests/status_test.cpp src/status.o src/player_store.o src/recipe_snapshot_store.o src/quest_snapshot_store.o src/engine/engine_core.o \
		src/engine/uobject.o src/engine/players.o src/engine/kills.o src/engine/resources.o src/engine/npcs.o src/engine/admin_teleport.o src/utils/memory.o \
	src/engine/admin_storage.o \
	src/engine/admin_gravestone.o \
	src/engine/player_recipes.o \
	src/engine/player_quests.o \
	src/utils/glibc_compat.o -o dist/status_test -pthread -ldl
	./dist/status_test
	$(CXX) $(CXXFLAGS) tests/admin_store_test.cpp src/admin_store.o -o dist/admin_store_test -pthread
	./dist/admin_store_test

.PHONY: all clean test
