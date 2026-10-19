CC      = cc
CFLAGS  = -O2 -Wall -Wextra -pthread
LDFLAGS = -pthread

TARGET  = lab2
OBJS    = runner.o timsort.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

runner.o: runner.c timsort.h
	$(CC) $(CFLAGS) -c runner.c -o $@

timsort.o: timsort.c timsort.h
	$(CC) $(CFLAGS) -c timsort.c -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean