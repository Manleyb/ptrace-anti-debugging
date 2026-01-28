CC = gcc
# Flags: -Wall (warnings), -std=gnu99 (standard Linux C)
CFLAGS = -Wall -std=gnu99

# The source file name (must match your file exactly)
SRC = ptrace-tracer.c

# The name of the final executable program
TARGET = ptrace-tracer

# Rule 1: Default build (Type 'make')
# Compiles with debug info (-g) so you can test/fix it.
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -g -o $(TARGET) $(SRC)

# Rule 2: Stealth build (Type 'make stealth')
# Compiles with -s to STRIP symbols (removes "main", variable names, etc)
# Use this for the "Release" version to make it harder to reverse.
stealth: $(SRC)
	$(CC) $(CFLAGS) -s -o $(TARGET) $(SRC)

# Clean up build files
clean:
	rm -f $(TARGET)
