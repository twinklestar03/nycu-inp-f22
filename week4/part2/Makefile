CC = g++
CFLAG = -lm
TARGET = tcpcbr
BUILD=build

all: $(TARGET)

$(TARGET): $(BUILD)/tcpcbr.o build
	@echo "Building target"
	$(CC) -g $< $(CFLAG) -o $@

$(BUILD)/tcpcbr.o: tcpcbr.c build
	
	$(CC) -c $< -o $@

build:
	mkdir -p $(BUILD)

clean:
	rm -rf *.o $(TARGET)

.PHONY: build clean