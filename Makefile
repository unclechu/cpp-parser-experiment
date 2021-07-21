PREFIX := /usr/local
PROGRAM_NAME := cpp-parsing
CXX := g++
CPP_STANDARD := c++17

all: clean build

clean:
	rm -fv -- '$(PROGRAM_NAME)'

build:
	'$(CXX)' -O2 -Wall -Wextra -std='$(CPP_STANDARD)' -o '$(PROGRAM_NAME)' main.cpp

run: build
	./'$(PROGRAM_NAME)'

install:
	cp -- '$(PROGRAM_NAME)' '$(PREFIX)/bin/$(PROGRAM_NAME)'
