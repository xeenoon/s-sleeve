#include "parser_internal.h"

#include <string.h>

static int token_matches_text(const token_t *token, const char *text) {
  size_t length = strlen(text);
  return token->length == length && strncmp(token->start, text, length) == 0;
}

static int token_is_significant(const token_t *token) {
  return token->kind != TOKEN_WHITESPACE && token->kind != TOKEN_COMMENT && token->kind != TOKEN_EOF;
}

static size_t parser_find_next_significant(const parser_state_t *state, size_t start_index) {
  size_t index;

  for (index = start_index; index < state->count; ++index) {
    const token_t *token = &state->tokens[index];
    if (token_is_significant(token)) {
      return index;
    }
  }

  return state->count;
}

static size_t parser_find_previous_significant(const parser_state_t *state, size_t start_index) {
  size_t index = start_index;

  while (index > 0) {
    index -= 1;
    if (token_is_significant(&state->tokens[index])) {
      return index;
    }
  }

  return state->count;
}

static int token_is_identifier_text(const token_t *token, const char *text) {
  return token->kind == TOKEN_IDENTIFIER && token_matches_text(token, text);
}

static void copy_token_text(char *buffer, size_t buffer_size, const token_t *token) {
  size_t length = token->length;
  if (length >= buffer_size) {
    length = buffer_size - 1;
  }
  memcpy(buffer, token->start, length);
  buffer[length] = '\0';
}

static void append_member_if_missing(ast_component_file_t *component,
                                     const token_t *token,
                                     ast_member_kind_t kind,
                                     int uses_external_fetch) {
  char name[128];
  size_t index;

  copy_token_text(name, sizeof(name), token);

  for (index = 0; index < component->member_count; ++index) {
    if (strcmp(component->members[index].name, name) == 0) {
      component->members[index].kind = kind;
      component->members[index].uses_external_fetch =
          component->members[index].uses_external_fetch || uses_external_fetch;
      component->members[index].is_observable =
          component->members[index].is_observable || strchr(name, '$') != NULL;
      return;
    }
  }

  if (component->member_count >= AST_MAX_MEMBERS) {
    return;
  }

  memcpy(component->members[component->member_count].name, name, sizeof(name));
  component->members[component->member_count].kind = kind;
  component->members[component->member_count].uses_external_fetch = uses_external_fetch;
  component->members[component->member_count].is_observable = strchr(name, '$') != NULL;
  component->members[component->member_count].param_count = 0;
  component->members[component->member_count].body[0] = '\0';
  component->members[component->member_count].initializer[0] = '\0';
  component->member_count += 1;
}

static ast_member_t *find_member(ast_component_file_t *component, const char *name) {
  size_t index;

  for (index = 0; index < component->member_count; ++index) {
    if (strcmp(component->members[index].name, name) == 0) {
      return &component->members[index];
    }
  }

  return NULL;
}

static size_t parser_find_matching_punctuation(const parser_state_t *state,
                                               size_t start_index,
                                               const char *open_text,
                                               const char *close_text) {
  size_t index;
  size_t depth = 0;

  for (index = start_index; index < state->count; ++index) {
    const token_t *token = &state->tokens[index];
    if (!token_is_significant(token)) {
      continue;
    }
    if (token->kind == TOKEN_PUNCTUATION && token_matches_text(token, open_text)) {
      depth += 1;
    } else if (token->kind == TOKEN_PUNCTUATION && token_matches_text(token, close_text)) {
      if (depth == 0) {
        return state->count;
      }
      depth -= 1;
      if (depth == 0) {
        return index;
      }
    }
  }

  return state->count;
}

static void parser_capture_method_signature(ast_member_t *member,
                                            const parser_state_t *state,
                                            size_t open_paren_index,
                                            size_t close_paren_index) {
  size_t index;

  member->param_count = 0;
  for (index = open_paren_index + 1; index < close_paren_index; ++index) {
    const token_t *token = &state->tokens[index];
    if (token->kind == TOKEN_IDENTIFIER) {
      if (member->param_count < AST_MAX_METHOD_PARAMS) {
        copy_token_text(member->params[member->param_count],
                        sizeof(member->params[member->param_count]),
                        token);
        member->param_count += 1;
      }
    }
  }
}

static void parser_capture_method_body(ast_member_t *member,
                                       const parser_state_t *state,
                                       size_t open_brace_index,
                                       size_t close_brace_index) {
  const char *body_start;
  const char *body_end;
  size_t length;

  if (open_brace_index >= state->count || close_brace_index >= state->count || close_brace_index <= open_brace_index) {
    member->body[0] = '\0';
    return;
  }

  body_start = state->tokens[open_brace_index].start + state->tokens[open_brace_index].length;
  body_end = state->tokens[close_brace_index].start;
  length = (size_t)(body_end - body_start);
  if (length >= sizeof(member->body)) {
    length = sizeof(member->body) - 1;
  }

  memcpy(member->body, body_start, length);
  member->body[length] = '\0';
}

static void parser_capture_field_initializer(ast_member_t *member,
                                             const parser_state_t *state,
                                             size_t equals_index) {
  size_t index;
  const char *start;
  const char *end;
  size_t length;

  if (equals_index >= state->count) {
    member->initializer[0] = '\0';
    return;
  }

  start = state->tokens[equals_index].start + state->tokens[equals_index].length;
  end = start;

  for (index = equals_index + 1; index < state->count; ++index) {
    const token_t *token = &state->tokens[index];

    if (!token_is_significant(token)) {
      continue;
    }

    if (token->kind == TOKEN_PUNCTUATION && token_matches_text(token, ";")) {
      end = token->start;
      break;
    }
  }

  if (end <= start) {
    member->initializer[0] = '\0';
    return;
  }

  length = (size_t)(end - start);
  if (length >= sizeof(member->initializer)) {
    length = sizeof(member->initializer) - 1;
  }

  memcpy(member->initializer, start, length);
  member->initializer[length] = '\0';
}

int parser_parse_component(parser_state_t *state, ast_file_t *out_file) {
  size_t index;
  int inside_class_body = 0;
  int waiting_for_class_body = 0;
  size_t class_depth = 0;
  int seen_class_keyword = 0;

  ast_file_init(out_file, PARSER_FILE_COMPONENT);

  for (index = 0; index < state->count; ++index) {
    const token_t *token = &state->tokens[index];
    size_t next_index;
    size_t next_next_index;
    size_t previous_index;
    const token_t *next_token;
    const token_t *next_next_token;
    const token_t *previous_token;
    char member_name[128];

    if (!token_is_significant(token) || token->kind == TOKEN_EOF) {
      continue;
    }

    if (token_is_identifier_text(token, "Component")) {
      out_file->data.component.has_component_decorator = 1;
    }

    if (seen_class_keyword && token->kind == TOKEN_IDENTIFIER) {
      copy_token_text(out_file->data.component.class_name,
                      sizeof(out_file->data.component.class_name),
                      token);
      seen_class_keyword = 0;
      waiting_for_class_body = 1;
      continue;
    }

    if (!inside_class_body && token_is_identifier_text(token, "class")) {
      seen_class_keyword = 1;
      continue;
    }

    if (token->kind == TOKEN_PUNCTUATION && token_matches_text(token, "{")) {
      if (waiting_for_class_body) {
        inside_class_body = 1;
        waiting_for_class_body = 0;
        class_depth = 1;
      } else if (inside_class_body) {
        class_depth += 1;
      }
    } else if (token->kind == TOKEN_PUNCTUATION && token_matches_text(token, "}")) {
      if (inside_class_body && class_depth > 0) {
        class_depth -= 1;
        if (class_depth == 0) {
          inside_class_body = 0;
        }
      }
      continue;
    }

    if (!inside_class_body || class_depth != 1) {
      continue;
    }

    next_index = parser_find_next_significant(state, index + 1);
    if (next_index >= state->count) {
      continue;
    }

    next_token = &state->tokens[next_index];
    previous_index = parser_find_previous_significant(state, index);
    previous_token = previous_index < state->count ? &state->tokens[previous_index] : NULL;

    if (token->kind == TOKEN_IDENTIFIER &&
        !(previous_token != NULL && token_is_identifier_text(previous_token, "async")) &&
        !(previous_token != NULL &&
          previous_token->kind == TOKEN_PUNCTUATION &&
          token_matches_text(previous_token, ".")) &&
        ((next_token->kind == TOKEN_PUNCTUATION && token_matches_text(next_token, "=")) ||
         (next_token->kind == TOKEN_PUNCTUATION && token_matches_text(next_token, "(")))) {
      append_member_if_missing(&out_file->data.component,
                               token,
                               token_matches_text(next_token, "(") ? AST_MEMBER_METHOD : AST_MEMBER_FIELD,
                               0);
      if (token_matches_text(next_token, "(")) {
        size_t close_paren_index = parser_find_matching_punctuation(state, next_index, "(", ")");
        size_t open_brace_index;
        size_t close_brace_index;
        ast_member_t *member;

        copy_token_text(member_name, sizeof(member_name), token);
        member = find_member(&out_file->data.component, member_name);
        if (member != NULL && close_paren_index < state->count) {
          parser_capture_method_signature(member, state, next_index, close_paren_index);
          open_brace_index = parser_find_next_significant(state, close_paren_index + 1);
          if (open_brace_index < state->count &&
              state->tokens[open_brace_index].kind == TOKEN_PUNCTUATION &&
              token_matches_text(&state->tokens[open_brace_index], "{")) {
            close_brace_index = parser_find_matching_punctuation(state, open_brace_index, "{", "}");
            if (close_brace_index < state->count) {
              parser_capture_method_body(member, state, open_brace_index, close_brace_index);
              index = close_brace_index;
              class_depth = 1;
            }
          }
        }
      } else if (token_matches_text(next_token, "=")) {
        ast_member_t *member;

        copy_token_text(member_name, sizeof(member_name), token);
        member = find_member(&out_file->data.component, member_name);
        if (member != NULL) {
          parser_capture_field_initializer(member, state, next_index);
        }
      }
      continue;
    }

    if (token_is_identifier_text(token, "async") && next_token->kind == TOKEN_IDENTIFIER) {
      next_next_index = parser_find_next_significant(state, next_index + 1);
      if (next_next_index < state->count) {
        next_next_token = &state->tokens[next_next_index];
        if (next_next_token->kind == TOKEN_PUNCTUATION && token_matches_text(next_next_token, "(")) {
          append_member_if_missing(&out_file->data.component,
                                   next_token,
                                   AST_MEMBER_METHOD,
                                   token_is_identifier_text(next_token, "refreshReading"));
          copy_token_text(member_name, sizeof(member_name), next_token);
          {
            size_t close_paren_index = parser_find_matching_punctuation(state, next_next_index, "(", ")");
            size_t open_brace_index;
            size_t close_brace_index;
            ast_member_t *member = find_member(&out_file->data.component, member_name);

            if (member != NULL && close_paren_index < state->count) {
              parser_capture_method_signature(member, state, next_next_index, close_paren_index);
              open_brace_index = parser_find_next_significant(state, close_paren_index + 1);
              if (open_brace_index < state->count &&
                  state->tokens[open_brace_index].kind == TOKEN_PUNCTUATION &&
                  token_matches_text(&state->tokens[open_brace_index], "{")) {
                close_brace_index = parser_find_matching_punctuation(state, open_brace_index, "{", "}");
                if (close_brace_index < state->count) {
                  parser_capture_method_body(member, state, open_brace_index, close_brace_index);
                  index = close_brace_index;
                  class_depth = 1;
                }
              }
            }
          }
        }
      }
    }
  }

  return 0;
}
