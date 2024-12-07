# Makefile for MWCp project

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -pthread

# Source files
SRCS = 210104004298_main.c

# Executable name
EXEC = 210104004298_main

# Default target
all: $(EXEC)

# Build the executable
$(EXEC): $(SRCS)
	$(CC) $(CFLAGS) -o $(EXEC) $(SRCS)

# Clean up the build files
clean:
	rm -f $(EXEC) *.o
	rm -rf ../tocopy/*

# Run the program with default parameters
run: $(EXEC)
	./$(EXEC) 10 4 ../testdir ../tocopy

# Run the tests
test1: clean $(EXEC)
	valgrind --leak-check=full ./$(EXEC) 10 10 ../testdir/src/libvterm ../tocopy

test2: clean $(EXEC)
	./$(EXEC) 10 4 ../testdir/src/libvterm/src ../tocopy

test3: clean $(EXEC)
	./$(EXEC) 10 10 ../testdir ../tocopy

.PHONY: all clean run test1 test2 test3
