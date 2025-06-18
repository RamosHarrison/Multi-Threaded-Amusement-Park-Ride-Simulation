CC = gcc
CFLAGS = -Wall -g -pthread
TARGET = park
VALGRIND_LOG = log3.txt
ARGS = -n 12 -c 2 -p 4 -w 6 -r 3

SRCS = park.c monitor.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c park.h
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) $(ARGS)

valgrind: $(TARGET)
	valgrind --leak-check=full --tool=memcheck ./$(TARGET) $(ARGS) > $(VALGRIND_LOG) 2>&1

clean:
	rm -f $(TARGET) *.o $(VALGRIND_LOG)

