CC = gcc
CFLAGS = -Iinclude -Wall
TARGET = bin/tish
SRC = $(wildcard src/*.c) $(wildcard src/builtins/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))

all: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p bin
	$(CC) $(OBJ) -o $(TARGET)

obj/%.o: src/%.c
	mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf obj/*.o $(TARGET)

run:
	make clean
	make all
	$(TARGET)

run_test:
	mkdir -p test
	$(CC) $(CFLAGS) -o ./test/test $(FILE)
	./test/test

clean_test:
	rm -rf ./test
