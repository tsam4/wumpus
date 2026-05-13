# Makefile — Wumpus Tracker (DE424 Mini-Project)
#
# All build targets delegate to the CMake project inside
# emdw_de424/devel/emdw/build/.  Adjust BUILD_DIR if your build
# directory lives elsewhere.
#
# Targets:
#   make all             — configure + build everything
#   make wumpus          — build only the tracker binary
#   make tests_basic     — build + run required unit tests
#   make tests_advanced  — build + run extended tests
#   make clean           — clean CMake build artefacts
#   make help            — list targets

BUILD_DIR := emdw_de424/devel/emdw/build
BIN_DIR   := $(BUILD_DIR)/src/bin

.PHONY: all wumpus tests_basic tests_advanced clean help

## Configure + build all targets
all:
	cd $(BUILD_DIR) && cmake ../ -DCMAKE_BUILD_TYPE=Release && make -j$$(nproc)

## Build only the wumpus binary
wumpus:
	cd $(BUILD_DIR) && cmake ../ -DCMAKE_BUILD_TYPE=Release && \
	  make -j$$(nproc) wumpus

## Build and run required basic unit tests
tests_basic:
	cd $(BUILD_DIR) && cmake ../ -DCMAKE_BUILD_TYPE=Release && \
	  make -j$$(nproc) test_transition test_emission test_parser test_emdw_bp
	@echo "--- Running tests_basic ---"
	$(BIN_DIR)/test_transition
	$(BIN_DIR)/test_emission
	$(BIN_DIR)/test_parser
	$(BIN_DIR)/test_emdw_bp
	@echo "--- All basic tests passed ---"

## Build and run extended tests (EM, e2e sim, scalability)
tests_advanced:
	cd $(BUILD_DIR) && cmake ../ -DCMAKE_BUILD_TYPE=Release && \
	  make -j$$(nproc) test_em_extremes test_e2e_sim test_dataset2_scale
	@echo "--- Running tests_advanced ---"
	$(BIN_DIR)/test_em_extremes
	$(BIN_DIR)/test_e2e_sim
	$(BIN_DIR)/test_dataset2_scale
	@echo "--- All advanced tests passed ---"

## Clean CMake build artefacts
clean:
	cd $(BUILD_DIR) && make clean

## List available targets
help:
	@grep -E '^##' $(MAKEFILE_LIST) | sed 's/^## //'
