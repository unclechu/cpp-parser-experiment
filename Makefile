PREFIX := /usr/local
PROGRAM_NAME := cpp-parsing
CXX := g++
CPP_STANDARD := c++17
BUILD_DIR := build

all: clean build

clean:
	rm -rfv -- '$(BUILD_DIR)'

build:
	mkdir -p -- '$(BUILD_DIR)'
	'$(CXX)' -O2 -Wall -Wextra -std='$(CPP_STANDARD)' \
		-o '$(BUILD_DIR)/$(PROGRAM_NAME)' src/main.cpp

run: build
	./'$(PROGRAM_NAME)'

install: build
	cp -- '$(BUILD_DIR)/$(PROGRAM_NAME)' '$(PREFIX)/bin/$(PROGRAM_NAME)'
