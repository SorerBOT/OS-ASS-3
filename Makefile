CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = 

SRCS = ex3.c Focus-Mode.c CPU-Scheduler.c
OBJS = $(SRCS:.c=.o)
TARGET = program

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
