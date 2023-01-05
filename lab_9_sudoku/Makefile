CC = g++
CFLAG = -lm -static
TARGET = solver
BUILD=build

all: $(TARGET)

$(TARGET): $(BUILD)/solver.o build
	@echo "Building target"
	$(CC) -g $< $(CFLAG) -o $@

$(BUILD)/solver.o: solver.cpp build
	
	$(CC) -c $< -o $@

build:
	mkdir -p $(BUILD)

clean:
	rm -rf *.o $(TARGET)

.PHONY: build clean