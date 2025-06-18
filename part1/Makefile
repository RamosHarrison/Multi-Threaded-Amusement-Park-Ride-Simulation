CC = gcc
CFLAGS = -Wall -g -pthread
TARGET = park
VALGRIND_LOG = log1.txt

all: $(TARGET)

$(TARGET): park.c
	$(CC) $(CFLAGS) -o $(TARGET) park.c

run: $(TARGET)
	./$(TARGET)

valgrind: $(TARGET)
	valgrind --leak-check=full --tool=memcheck ./$(TARGET) > $(VALGRIND_LOG) 2>&1
clean:
	rm -f $(TARGET) *.o $(VALGRIND_LOG)
