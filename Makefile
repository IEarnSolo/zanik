CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -fPIC -fvisibility=hidden -Wall -Wextra -Wno-unused-parameter -Isrc -U_GNU_SOURCE -D_DEFAULT_SOURCE -MMD -MP
LDFLAGS ?= -shared -pthread -ldl -static-libstdc++ -static-libgcc
PYTHON ?= python3
NODE ?= node

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
	rm -f $(OBJ) $(DEP) $(OUT) dist/zanik-compat-analyzer dist/status_test dist/admin_store_test
	$(MAKE) -C qol clean

-include $(DEP)

test: test-core

test-core: $(OUT) dist/zanik-compat-analyzer
	mkdir -p dist
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
	$(MAKE) -C qol test
	$(PYTHON) tests/player_recipe_links_test.py
	$(PYTHON) tests/recipe_category_navigation_test.py
	$(PYTHON) tests/recipe_npc_building_compatibility_test.py
	$(PYTHON) tests/recipe_tree_wiki_links_test.py
	$(PYTHON) tests/gitlab_pipeline_monitor_test.py
	$(PYTHON) tests/release_automation_test.py
	$(PYTHON) tests/github_release_test.py
	$(PYTHON) tests/game_update_25811348_test.py
	$(PYTHON) tests/game_update_25991736_test.py
	$(PYTHON) tests/quest_schema_test.py
	sh tests/config_test.sh
	$(PYTHON) tests/asset_tool_test.py
	$(PYTHON) tests/asset_delta_test.py
	$(PYTHON) tests/generate_web_assets_test.py
	$(PYTHON) tests/recipe_icon_resolution_test.py
	$(PYTHON) tests/windows_asset_pipeline_test.py
	$(PYTHON) tests/windows_mapping_test.py
	$(PYTHON) tests/manager_test.py
	$(PYTHON) tests/game_update_safety_test.py
	$(PYTHON) tests/update_awareness_test.py
	sh tests/installer_test.sh
	sh tests/server_asset_verifier_test.sh
	sh tests/server_game_update_test.sh
	$(NODE) tests/map_page_test.js
	$(PYTHON) tests/http_core_test.py
	$(PYTHON) tests/maintenance_http_test.py
	$(PYTHON) tests/feature_gate_http_test.py
	sh tests/updater_test.sh
	sh tests/launcher_game_update_test.sh
	sh tests/compatible_release_check_test.sh

test-assets:
	$(PYTHON) tests/require_game_assets.py
	$(MAKE) $(OUT)
	$(PYTHON) tests/building_catalog_test.py
	$(PYTHON) tests/npc_catalog_test.py
	$(PYTHON) tests/plan_item_catalog_test.py
	$(PYTHON) tests/quest_catalog_test.py
	$(PYTHON) tests/test_recipe_unlock_catalog.py
	$(PYTHON) tests/http_test.py

test-all:
	$(MAKE) test-core
	$(MAKE) test-assets

package: all
	sh scripts/package-release.sh

validate-package: package
	$(PYTHON) tools/validate_release.py --directory dist/release

.PHONY: all clean test test-core test-assets test-all package validate-package
