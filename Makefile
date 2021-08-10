PREFIX := /usr/local
TARGET := cpp-parsing
BUILD_DIR := build

CPP_STANDARD := c++17
CXX := g++
CXX_FLAGS := -O2 -Wall -Wextra -std='$(CPP_STANDARD)' -Isrc

SRC := $(wildcard src/*.cpp src/*/*.cpp src/*/*/*.cpp)
OBJ := $(patsubst %.cpp,%.o,$(patsubst src/%,$(BUILD_DIR)/%,$(SRC)))

all: clean build

clean:
	rm -rfv -- '$(BUILD_DIR)'

$(BUILD_DIR)/%.o: src/%.cpp
	mkdir -pv -- '$(dir $@)'
	'$(CXX)' $(CXX_FLAGS) -o '$@' -c '$<'

build: $(OBJ)
	mkdir -pv -- '$(BUILD_DIR)'
	'$(CXX)' $(CXX_FLAGS) -o '$(BUILD_DIR)/$(TARGET)' $(OBJ)

run: build
	'$(BUILD_DIR)/$(TARGET)' < example.json

test: build
	'$(BUILD_DIR)/$(TARGET)' test
	'$(BUILD_DIR)/$(TARGET)' < example.json | bash test-json.sh
	'$(BUILD_DIR)/$(TARGET)' --pretty < example.json | bash test-json.sh
	'$(BUILD_DIR)/$(TARGET)' --model < example.json | bash test-json.sh --model
	'$(BUILD_DIR)/$(TARGET)' --model --pretty < example.json | bash test-json.sh --model

install: build
	cp -- '$(BUILD_DIR)/$(TARGET)' '$(PREFIX)/bin/$(TARGET)'

info:
	@echo "SRC: ${SRC}"
	@echo "OBJ: ${OBJ}"
