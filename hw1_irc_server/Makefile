BIN     := ./bin
SRC     := ./src

MKDIR   := mkdir

CC      := g++
INCLUDE := -Iinclude
LIB     := 
CFLAGS  := -Wall -std=c++14 -g

HANDLER_DIR := src/handlers
HANDLER    := $(shell find $(HANDLER_DIR) -name '*.cpp')

MANAGER_DIR := src/managers
MANAGER    := $(shell find $(MANAGER_DIR) -name '*.cpp')

MAIN    := $(shell find $(SRC) -name '*.cpp')

SRCS    := $(MANAGER) $(HANDLER) $(MAIN)
OBJS    := $(SRCS)

DEPS := $(OBJS:%.cpp=%.d)
OBJS := $(OBJS:%.cpp=%.o)

EXE     := tinyircd

.PHONY: all run clean

all: $(EXE)

$(EXE): $(OBJS) | $(BIN)
	$(CC) $^ -o $@ $(LIB)

%.o: %.cpp
	$(CC) -o $@ $(CFLAGS) $(INCLUDE) -c -MMD $<

$(BIN) $(OBJ):
	$(MKDIR) $@

run: $(EXE)
	$<

clean:
	$(RM) $(DEPS) $(OBJS) $(EXE)