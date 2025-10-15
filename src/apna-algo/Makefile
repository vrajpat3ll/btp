# Paths
SRC = src
INCLUDE = include

# Source files
SRCS = $(SRC)/amf.c \
       $(SRC)/forward.c \
       main.c \
       $(SRC)/scale.c \
       $(SRC)/utils.c

# Object files
OBJS = $(SRCS:.c=.o)

# Compiler and flags
CC = gcc
CFLAGS = -I$(INCLUDE) -Wall -Wextra

# Target executable
TARGET = lb

# Default target
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) -lsctp

$(SRC)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SRC)/*.o $(TARGET)

.PHONY: all clean
