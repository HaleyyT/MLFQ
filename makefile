# Compiler and common flags
CC           = gcc
WARNFLAGS    = -Wall -Wextra -Wshadow -Wconversion -pedantic
STDFLAGS     = -std=c11
OPTFLAGS     = -O2
DBGFLAGS     = -g
THREADFLAGS  = -pthread
DEPFLAGS     = -MMD -MP

# Sanitizers (enabled with SAN=1, e.g., `make SAN=1`)
ASAN_CFLAGS  = -fsanitize=address -fno-omit-frame-pointer -O0
ASAN_LDFLAGS = -fsanitize=address

# Directories
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin
BIN     = $(BIN_DIR)/sim
SIGTASK = $(BIN_DIR)/sigtrap

# Sources / objects / deps
# NOTE: exclude src/sigtrap.c from the main sources, because it is a separate program
SRC  = $(filter-out $(SRC_DIR)/sigtrap.c, $(wildcard $(SRC_DIR)/*.c))
OBJ  = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))
DEPS = $(OBJ:.o=.d)

# Compose CFLAGS/LDFLAGS
CFLAGS  = $(STDFLAGS) $(WARNFLAGS) $(OPTFLAGS) $(DBGFLAGS) $(THREADFLAGS) $(DEPFLAGS) -I$(INC_DIR)
LDFLAGS = $(THREADFLAGS)

ifeq ($(SAN),1)
  CFLAGS  += $(ASAN_CFLAGS)
  LDFLAGS += $(ASAN_LDFLAGS)
endif

# Default target: build child first (dispatcher execs it), then dispatcher
all: $(SIGTASK) $(BIN)

# Build the child process program that the dispatcher will exec
$(SIGTASK): $(SRC_DIR)/sigtrap.c | $(BIN_DIR)
	$(CC) -o $@ $<

# Link main dispatcher
$(BIN): $(OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

# Compile (with header dependency files)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Ensure dirs exist
$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

# Convenience targets
run: $(BIN)
	./$(BIN)

asan:  ## build with AddressSanitizer (same as `make SAN=1`)
	$(MAKE) SAN=1 all

run-asan:
	ASAN_OPTIONS="strict_string_checks=1:abort_on_error=1:fast_unwind_on_malloc=0" $(MAKE) SAN=1 run

rebuild: clean all

clean:
	$(RM) -r $(OBJ_DIR) $(BIN_DIR)

# Include auto-generated dependency files
-include $(DEPS)

.PHONY: all clean run asan run-asan rebuild
