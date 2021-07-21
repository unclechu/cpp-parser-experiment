PREFIX := /usr/local
PROGRAM_NAME := cpp-parsing
BUILD_DIR := build

CPP_STANDARD := c++17
CXX := g++
CXX_FLAGS := -O2 -Wall -Wextra -std='$(CPP_STANDARD)'

all: clean build

clean:
	rm -rfv -- '$(BUILD_DIR)'

build:
	mkdir -pv -- '$(BUILD_DIR)'
	'$(CXX)' $(CXX_FLAGS) -o '$(BUILD_DIR)/$(PROGRAM_NAME)' src/main.cpp

run: build
	./'$(PROGRAM_NAME)'

install: build
	cp -- '$(BUILD_DIR)/$(PROGRAM_NAME)' '$(PREFIX)/bin/$(PROGRAM_NAME)'
