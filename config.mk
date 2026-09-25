# Final output file basename
TARGET_NAME := pc-engine-emu

# Compile as a shared library vs a standalone executable
# This will define _SHARED in $(CC) if enabled
SHARED ?= 0

# Compiler and linker flags
CFLAGS ?=
LDFLAGS ?=

# Bundle additional debugging stuff with the final output file
# DEBUG also defines _DEBUG in $(CC)
DEBUG ?= 1
ASAN ?= 1

# Libraries to link with the final output file
LIBRARIES :=
LIBS_PKGCONF :=

# Makefile logging mode (1 to enable logging all commands)
VERBOSE ?= 0
