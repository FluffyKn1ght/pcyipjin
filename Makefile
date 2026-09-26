.DEFAULT_GOAL := all
.PHONY: all clean rebuild cc_cmds

SRC_DIR := src
INCLUDE_DIR := include
BIN_DIR := bin
BUILD_DIR := build

include config.mk

ifneq ($(shell uname -m),x86_64)
$(error screw ur $(shell uname -m) ass bro)
endif

ifneq ($(VERBOSE),1)
V := @
endif

ifeq ($(SHARED),1)
CFLAGS += -fPIC -D_SHARED
LDFLAGS += -shared
ifeq ($(OS),Windows_NT)
TARGET := $(BUILD_DIR)/$(TARGET_NAME).dll
else
TARGET := $(BUILD_DIR)/$(TARGET_NAME).so
endif
else
ifeq ($(OS),Windows_NT)
TARGET := $(BUILD_DIR)/$(TARGET_NAME).exe
else
TARGET := $(BUILD_DIR)/$(TARGET_NAME).$(shell uname -m)
endif
endif

ifeq ($(DEBUG),1)
CFLAGS += -g
LDFLAGS += -g
endif

ifeq ($(ASAN),1)
CFLAGS += -fsanitize=address
LDFLAGS += -fsanitize=address
endif

CFLAGS += $(patsubst %,-l%,$(LIBRARIES))
LDFLAGS += $(patsubst %,-l%,$(LIBRARIES))

ifneq ($(LIBS_PKGCONF),)
CFLAGS += $(shell pkgconf --cflags $(LIBS_PKGCONF))
LDFLAGS += $(shell pkgconf --libs $(LIBS_PKGCONF))
endif

CFLAGS += -std=gnu23 -pedantic -masm=intel

C_SOURCES := $(shell find $(SRC_DIR) -type f -name "*.c")
C_OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(C_SOURCES))

ASM_SOURCES := $(shell find $(SRC_DIR) -type f -name "*.s")
ASM_OBJECTS := $(patsubst $(SRC_DIR)/%.s,$(BIN_DIR)/%.o,$(ASM_SOURCES))

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	$(V)mkdir -p $(shell dirname $@)
	@echo -e "Compiling: $^ ==> $@"
	$(V)$(CC) -c -I"./$(INCLUDE_DIR)" -include "./$(INCLUDE_DIR)/preinc.h" $(CFLAGS) -o $@ $^

$(BIN_DIR)/%.o: $(SRC_DIR)/%.s
	$(V)mkdir -p $(shell dirname $@)
	@echo -e "Assembling: $^ ==> $@"
	$(V)$(CC) -c $(ASMFLAGS) -o $@ $^

$(TARGET): $(C_OBJECTS) $(ASM_OBJECTS)
	$(V)@mkdir -p $(shell dirname $@)
	@echo -e "Linking: $@"
	$(V)$(CC) $(LDFLAGS) -o $@ $^

cc_cmds:
	$(V)bear -- $(CC) -xc -c -fsyntax-only -I"./$(INCLUDE_DIR)" -include "./$(INCLUDE_DIR)/preinc.h" $(CFLAGS) dummy.c
	@echo -e "Generating: compile_commands.json"

all: $(TARGET) cc_cmds

clean:
	@echo "Cleaning build output folders"
	-rm -rf ./$(BIN_DIR)/*
	-rm -rf ./$(BUILD_DIR)/*

rebuild: clean
	$(MAKE)
