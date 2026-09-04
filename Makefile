CC = gcc
CFLAGS = -Wall -g
SRCS = $(wildcard *.c)
DEPS = $(wildcard *.h)
OBJ = $(SRCS:.c=.o)
TARGET = OWR
IP := $(shell hostname -I | awk '{print $$1}')

all: $(TARGET)


$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

run: $(TARGET)
	@$(eval PORT_SUFFIX=00)
	./$(TARGET) $(IP) 580$(PORT_SUFFIX)

run%: $(TARGET)
	@$(eval PORT_SUFFIX=$(shell printf "%02d" $*))
	./$(TARGET) $(IP) 580$(PORT_SUFFIX)

runt%: $(TARGET)
	@$(eval PORT_SUFFIX=$(shell printf "%02d" $*))
	./$(TARGET) $(IP) 580$(PORT_SUFFIX) 192.168.1.1 58141

val: $(TARGET)
	valgrind --leak-check=full --track-origins=yes --show-leak-kinds=all ./$(TARGET) $(IP) $(PORT)
clean:
	rm -f *.o $(TARGET)

.PHONY: all clean

