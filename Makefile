# ── Compiler ──────────────────────────────────────────────────────────────────
CXX      = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -g

# ── Directory structure ───────────────────────────────────────────────────────
SRC_DIR     = src/main
INCLUDE_DIR = src/include
OBJ_DIR     = obj

# ── Collect all C++ source files ─────────────────────────────────────────────
SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))
OBJ_MAIN  := $(OBJ_DIR)/main.o

# ── Target binary ─────────────────────────────────────────────────────────────
TARGET = sim

# ── Default target ────────────────────────────────────────────────────────────
all: $(OBJ_DIR) $(TARGET)

# ── Create obj/ directory if it doesn't exist ────────────────────────────────
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# ── Link all object files into the binary ────────────────────────────────────
$(TARGET): $(OBJ_FILES) $(OBJ_MAIN)
	$(CXX) $(CXXFLAGS) -o $@ $^

# ── Compile each source file in src/main/ ────────────────────────────────────
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# ── Compile main.cpp (project root) ──────────────────────────────────────────
$(OBJ_DIR)/main.o: main.cpp
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# ── Clean build artifacts ─────────────────────────────────────────────────────
clean:
	rm -rf $(OBJ_DIR)/*.o $(TARGET)

# ── Full clean rebuild ────────────────────────────────────────────────────────
rebuild: clean all

.PHONY: all clean rebuild