PREFIX := /usr/local
PROGRAM_NAME := cpp-parsing

all: clean build

clean:
	rm -fv "$(PROGRAM_NAME)"

build:
	g++ -O2 -Wall -Wextra -std=c++17 -o "$(PROGRAM_NAME)" main.cpp

run: build
	./"$(PROGRAM_NAME)"

install:
	cp -- "$(PROGRAM_NAME)" "$(PREFIX)/bin/$(PROGRAM_NAME)"
