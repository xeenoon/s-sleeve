SHELL := cmd.exe
.SHELLFLAGS := /C

CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -Iinclude -Isrc/tokenizer/include -Isrc/parser/include
DEPFLAGS := -MMD -MP
BIN_DIR := bin
SRC_DIR := src
OBJ_DIR := build
TARGET := $(BIN_DIR)/main.exe

SOURCES := \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/cli.c \
	$(SRC_DIR)/asset_writer.c \
	$(SRC_DIR)/file_io.c \
	$(SRC_DIR)/generator.c \
	$(SRC_DIR)/log.c \
	$(SRC_DIR)/output_fs.c \
	$(SRC_DIR)/path_scan.c \
	src/tokenizer/src/token.c \
	src/tokenizer/src/tokenizer.c \
	src/parser/src/ast.c \
	src/parser/src/parser.c \
	src/parser/src/parser_component.c \
	src/parser/src/parser_template.c \
	src/parser/src/parser_style.c

OBJECTS := \
	$(OBJ_DIR)/main.o \
	$(OBJ_DIR)/cli.o \
	$(OBJ_DIR)/asset_writer.o \
	$(OBJ_DIR)/file_io.o \
	$(OBJ_DIR)/generator.o \
	$(OBJ_DIR)/log.o \
	$(OBJ_DIR)/output_fs.o \
	$(OBJ_DIR)/path_scan.o \
	$(OBJ_DIR)/token.o \
	$(OBJ_DIR)/tokenizer.o \
	$(OBJ_DIR)/ast.o \
	$(OBJ_DIR)/parser.o \
	$(OBJ_DIR)/parser_component.o \
	$(OBJ_DIR)/parser_template.o \
	$(OBJ_DIR)/parser_style.o

DEPENDENCIES := $(OBJECTS:.o=.d)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: src/tokenizer/src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: src/parser/src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BIN_DIR):
	if not exist $(BIN_DIR) mkdir $(BIN_DIR)

$(OBJ_DIR):
	if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)

run: $(TARGET)
	./$(TARGET) ../angular_app

clean:
	if exist $(OBJ_DIR) rmdir /S /Q $(OBJ_DIR)
	if exist $(BIN_DIR) rmdir /S /Q $(BIN_DIR)

-include $(DEPENDENCIES)
