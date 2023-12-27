CXX = clang++
CXXFLAGS = -Wall -g -O3
CXXFLAGS += `llvm-config --cxxflags --ldflags --system-libs --libs core orcjit native`
CXXFLAGS += -Xlinker --export-dynamic

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

SRC_FILES := $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))

TARG = $(BIN_DIR)/inhu

.PHONY: all clean

all: $(TARG)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARG): $(OBJ_FILES) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
