SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
TARGET = paradox

.PHONY: all
all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
