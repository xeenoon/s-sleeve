SHELL := cmd.exe
.SHELLFLAGS := /C

CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -I.. -Iinclude -Iinclude\\data -Iinclude\\support
DEPFLAGS := -MMD -MP
LDFLAGS_COMMON := -lm
LDFLAGS_NET := -lws2_32

BIN_DIR := bin
BUILD_DIR := build

HELPER_SOURCES := \
	src/math/number_utils.c \
	src/math/ng_math.c \
	src/format/number_format.c \
	src/io/fetch_runtime.c \
	src/runtime/app_runtime.c \
	src/runtime/observable.c \
	src/data/json_utils.c \
	src/data/json.c \
	src/support/stringbuilder.c \
	src/support/test.c \
	src/net/http_server.c \
	src/net/http_service.c

UNIT_TEST_SOURCES := \
	tests/unit/test_main.c \
	tests/unit/test_number_utils.c \
	tests/unit/test_ng_math.c \
	tests/unit/test_number_format.c \
	tests/unit/test_fetch_runtime.c \
	tests/unit/test_app_runtime.c \
	tests/unit/test_observable.c \
	tests/unit/test_json_utils.c \
	tests/unit/test_http_service.c

INTEGRATION_SOURCES := tests/integration/helper_server_main.c

HELPER_OBJECTS := $(HELPER_SOURCES:%.c=$(BUILD_DIR)/%.o)
UNIT_TEST_OBJECTS := $(UNIT_TEST_SOURCES:%.c=$(BUILD_DIR)/%.o)
INTEGRATION_OBJECTS := $(INTEGRATION_SOURCES:%.c=$(BUILD_DIR)/%.o)
UNIT_OBJECTS := $(HELPER_OBJECTS)

UNIT_TEST_TARGET := $(BIN_DIR)/unit_tests.exe
HELPER_SERVER_TARGET := $(BIN_DIR)/helper_server.exe

DEPENDENCIES := $(HELPER_OBJECTS:.o=.d) $(UNIT_TEST_OBJECTS:.o=.d) $(INTEGRATION_OBJECTS:.o=.d)

.PHONY: all clean test integration prepare_dirs

all: $(UNIT_TEST_TARGET) $(HELPER_SERVER_TARGET)

test: $(UNIT_TEST_TARGET)
	.\\$(subst /,\\,$(UNIT_TEST_TARGET))

integration: $(HELPER_SERVER_TARGET)

$(UNIT_TEST_TARGET): $(UNIT_OBJECTS) $(UNIT_TEST_OBJECTS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(UNIT_OBJECTS) $(UNIT_TEST_OBJECTS) $(LDFLAGS_COMMON) $(LDFLAGS_NET)

$(HELPER_SERVER_TARGET): $(HELPER_OBJECTS) $(INTEGRATION_OBJECTS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(HELPER_OBJECTS) $(INTEGRATION_OBJECTS) $(LDFLAGS_COMMON) $(LDFLAGS_NET)

$(BUILD_DIR)/%.o: %.c | prepare_dirs
	$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

$(BIN_DIR):
	if not exist $(BIN_DIR) mkdir $(BIN_DIR)

prepare_dirs: $(BUILD_DIR)

$(BUILD_DIR):
	if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
	if not exist $(BUILD_DIR)\\src mkdir $(BUILD_DIR)\\src
	if not exist $(BUILD_DIR)\\src\\app mkdir $(BUILD_DIR)\\src\\app
	if not exist $(BUILD_DIR)\\src\\data mkdir $(BUILD_DIR)\\src\\data
	if not exist $(BUILD_DIR)\\src\\format mkdir $(BUILD_DIR)\\src\\format
	if not exist $(BUILD_DIR)\\src\\io mkdir $(BUILD_DIR)\\src\\io
	if not exist $(BUILD_DIR)\\src\\math mkdir $(BUILD_DIR)\\src\\math
	if not exist $(BUILD_DIR)\\src\\net mkdir $(BUILD_DIR)\\src\\net
	if not exist $(BUILD_DIR)\\src\\runtime mkdir $(BUILD_DIR)\\src\\runtime
	if not exist $(BUILD_DIR)\\src\\support mkdir $(BUILD_DIR)\\src\\support
	if not exist $(BUILD_DIR)\\tests mkdir $(BUILD_DIR)\\tests
	if not exist $(BUILD_DIR)\\tests\\unit mkdir $(BUILD_DIR)\\tests\\unit
	if not exist $(BUILD_DIR)\\tests\\integration mkdir $(BUILD_DIR)\\tests\\integration

clean:
	if exist $(BUILD_DIR) rmdir /S /Q $(BUILD_DIR)
	if exist $(BIN_DIR) rmdir /S /Q $(BIN_DIR)

-include $(DEPENDENCIES)
