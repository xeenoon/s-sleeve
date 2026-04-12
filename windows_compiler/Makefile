SHELL := cmd.exe
.SHELLFLAGS := /C

CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -Iinclude -Isrc/tokenizer/include -Isrc/parser/include -Ihelpers/include -Ihelpers/include/data -Ihelpers/include/support -Ihelpers/include/net -Ihelpers/include/runtime
DEPFLAGS := -MMD -MP
BIN_DIR := bin
SRC_DIR := src
OBJ_DIR := build
TARGET := $(BIN_DIR)/main.exe
TEST_TARGET := $(BIN_DIR)/tests.exe
TEST_LDFLAGS := -lm -lws2_32

SOURCES := \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/cli.c \
	$(SRC_DIR)/component_compose.c \
	$(SRC_DIR)/component_registry.c \
	$(SRC_DIR)/file_io.c \
	$(SRC_DIR)/ejs_codegen.c \
	$(SRC_DIR)/ejs_parser.c \
	$(SRC_DIR)/generator.c \
	$(SRC_DIR)/js_codegen.c \
	$(SRC_DIR)/log.c \
	$(SRC_DIR)/output_fs.c \
	$(SRC_DIR)/path_scan.c \
	$(SRC_DIR)/server_codegen.c \
	$(SRC_DIR)/server_parser.c \
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
	$(OBJ_DIR)/component_compose.o \
	$(OBJ_DIR)/component_registry.o \
	$(OBJ_DIR)/file_io.o \
	$(OBJ_DIR)/ejs_codegen.o \
	$(OBJ_DIR)/ejs_parser.o \
	$(OBJ_DIR)/generator.o \
	$(OBJ_DIR)/js_codegen.o \
	$(OBJ_DIR)/log.o \
	$(OBJ_DIR)/output_fs.o \
	$(OBJ_DIR)/path_scan.o \
	$(OBJ_DIR)/server_codegen.o \
	$(OBJ_DIR)/server_parser.o \
	$(OBJ_DIR)/token.o \
	$(OBJ_DIR)/tokenizer.o \
	$(OBJ_DIR)/ast.o \
	$(OBJ_DIR)/parser.o \
	$(OBJ_DIR)/parser_component.o \
	$(OBJ_DIR)/parser_template.o \
	$(OBJ_DIR)/parser_style.o

DEPENDENCIES := $(OBJECTS:.o=.d)

TEST_SOURCES := \
	tests/test_main.c \
	tests/test_component_registry.c \
	tests/test_component_compose.c \
	tests/test_server_backend.c \
	tests/test_server_runtime.c

TEST_OBJECTS := \
	$(OBJ_DIR)/test_main.o \
	$(OBJ_DIR)/test_component_registry.o \
	$(OBJ_DIR)/test_component_compose.o \
	$(OBJ_DIR)/test_server_backend.o \
	$(OBJ_DIR)/test_server_runtime.o \
	$(OBJ_DIR)/component_compose.o \
	$(OBJ_DIR)/component_registry.o \
	$(OBJ_DIR)/ejs_codegen.o \
	$(OBJ_DIR)/ejs_parser.o \
	$(OBJ_DIR)/file_io.o \
	$(OBJ_DIR)/log.o \
	$(OBJ_DIR)/output_fs.o \
	$(OBJ_DIR)/path_scan.o \
	$(OBJ_DIR)/server_codegen.o \
	$(OBJ_DIR)/server_parser.o \
	$(OBJ_DIR)/token.o \
	$(OBJ_DIR)/tokenizer.o \
	$(OBJ_DIR)/ast.o \
	$(OBJ_DIR)/parser.o \
	$(OBJ_DIR)/parser_component.o \
	$(OBJ_DIR)/parser_template.o \
	$(OBJ_DIR)/parser_style.o \
	$(OBJ_DIR)/helper_json.o \
	$(OBJ_DIR)/helper_http_server.o \
	$(OBJ_DIR)/helper_server_runtime.o \
	$(OBJ_DIR)/helper_stringbuilder.o

TEST_DEPENDENCIES := $(TEST_OBJECTS:.o=.d)

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

$(TEST_TARGET): $(TEST_OBJECTS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(TEST_OBJECTS) $(TEST_LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: src/tokenizer/src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: src/parser/src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: tests/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJ_DIR)/helper_json.o: helpers/src/data/json.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJ_DIR)/helper_http_server.o: helpers/src/net/http_server.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJ_DIR)/helper_server_runtime.o: helpers/src/runtime/server_runtime.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(OBJ_DIR)/helper_stringbuilder.o: helpers/src/support/stringbuilder.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BIN_DIR):
	if not exist $(BIN_DIR) mkdir $(BIN_DIR)

$(OBJ_DIR):
	if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)

run: $(TARGET)
	bin\main.exe ..\angular_app

test: $(TEST_TARGET)
	bin\tests.exe

clean:
	if exist $(OBJ_DIR) rmdir /S /Q $(OBJ_DIR)
	if exist $(BIN_DIR) rmdir /S /Q $(BIN_DIR)

-include $(DEPENDENCIES)
-include $(TEST_DEPENDENCIES)
