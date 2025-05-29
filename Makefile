CC = gcc
CFLAGS = -std=c99 -Wall -Wextra
TARGET = searchword
SRC = searchword.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)