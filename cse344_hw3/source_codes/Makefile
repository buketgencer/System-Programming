CC=gcc
CFLAGS=-Wall
TARGET=parking_system

all: $(TARGET) run

$(TARGET): parking_system.c
	$(CC) $(CFLAGS) parking_system.c -o $(TARGET) -pthread

run: 
	./$(TARGET)

clean:
	rm -f $(TARGET)
