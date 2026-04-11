#ifndef ANGULAR_RUNTIME_H
#define ANGULAR_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  NG_BINDING_TEXT = 0,
  NG_BINDING_ATTRIBUTE = 1,
  NG_BINDING_EVENT = 2
} ng_binding_kind_t;

typedef struct {
  const char *target_id;
  const char *target_name;
  const char *source_field;
  ng_binding_kind_t kind;
} ng_binding_t;

typedef struct {
  const char *selector;
  const char *template_html;
  const char *styles_css;
  const ng_binding_t *bindings;
  size_t binding_count;
} ng_component_def_t;

typedef struct {
  int reading;
  int percent_straight;
  float normalized;
  float knee_angle_deg;
  float ankle_x;
  float ankle_y;
  bool has_signal;
} ng_app_state_t;

typedef struct {
  float upper_leg_length;
  float lower_leg_length;
  float hip_x;
  float hip_y;
  float knee_x;
  float knee_y;
} ng_knee_visualizer_state_t;

typedef struct {
  int raw_reading;
  int display_percent;
  bool show_error;
} ng_reading_display_state_t;

typedef void (*ng_init_fn_t)(void *state);
typedef void (*ng_recompute_fn_t)(void *state);
typedef void (*ng_render_fn_t)(const void *state, char *buffer, size_t buffer_size);

typedef struct {
  ng_component_def_t def;
  size_t state_size;
  ng_init_fn_t init;
  ng_recompute_fn_t recompute;
  ng_render_fn_t render;
} ng_component_runtime_t;

#endif
