SHELL := cmd.exe
.SHELLFLAGS := /C

CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -I. -Ihelpers/include -Ihelpers/include/data -Ihelpers/include/support
LDFLAGS := -lm -lws2_32
BIN_DIR := bin
TARGET := $(BIN_DIR)/generated_demo.exe

SOURCES := \
	angular_selectedView.c \
	angular_goal.c \
	angular_reading.c \
	angular_percentStraight.c \
	angular_speed.c \
	angular_lastScore.c \
	angular_stepCount.c \
	angular_todayAverage.c \
	angular_timeSynced.c \
	angular_inStep.c \
	angular_shaky.c \
	angular_uncontrolledDescent.c \
	angular_compensation.c \
	angular_lowerLegX2.c \
	angular_lowerLegY2.c \
	angular_ankleX.c \
	angular_ankleY.c \
	angular_ngOnInit.c \
	angular_switchView.c \
	angular_clamp.c \
	angular_mapPercentToAngle.c \
	angular_updateKnee.c \
	angular_http_service.c \
	angular_generated_demo.c \
	helpers/src/data/json.c \
	helpers/src/data/json_utils.c \
	helpers/src/format/number_format.c \
	helpers/src/io/fetch_runtime.c \
	helpers/src/math/ng_math.c \
	helpers/src/math/number_utils.c \
	helpers/src/net/http_server.c \
	helpers/src/net/http_service.c \
	helpers/src/runtime/app_runtime.c \
	helpers/src/support/stringbuilder.c

.PHONY: all clean run

all: $(TARGET)

$(TARGET): | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(LDFLAGS)

$(BIN_DIR):
	if not exist $(BIN_DIR) mkdir $(BIN_DIR)

run: $(TARGET)
	.\\$(subst /,\\,$(TARGET)) 18080 0

clean:
	if exist $(BIN_DIR) rmdir /S /Q $(BIN_DIR)
