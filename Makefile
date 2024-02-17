CC = gcc
TARGET = bash
SRCS = bash.c bash_func.c history.c jobs.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $^ -o $@

%.o: %.c
	$(CC) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
