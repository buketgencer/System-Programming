# Compiler and linker configurations
CC = gcc
CFLAGS = -Wall -g
LDFLAGS =

# Define targets
all: clientx serverx

# Client and server executables
clientx: clientx.o logfile.o
	$(CC) $(LDFLAGS) -o $@ $^

serverx: serverx.o logfile.o
	$(CC) $(LDFLAGS) -o $@ $^

# Object files
clientx.o: clientx.c clientx.h logfile.h
	$(CC) $(CFLAGS) -c clientx.c

serverx.o: serverx.c serverx.h logfile.h
	$(CC) $(CFLAGS) -c serverx.c

logfile.o: logfile.c logfile.h
	$(CC) $(CFLAGS) -c logfile.c

# Clean old builds
clean:
	rm -f *.o clientx serverx
