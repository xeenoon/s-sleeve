#include "json.h"

#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct json_stringbuilder
{
    char *data;
    size_t written_len;
    size_t allocated;
} json_stringbuilder;

static json_stringbuilder *json_builder_init(void)
{
    json_stringbuilder *builder = (json_stringbuilder *)malloc(sizeof(json_stringbuilder));
    if (!builder)
        return NULL;
    builder->allocated = 64;
    builder->written_len = 0;
    builder->data = (char *)malloc(builder->allocated);
    if (!builder->data)
    {
        free(builder);
        return NULL;
    }
    return builder;
}

static void json_builder_free(json_stringbuilder *builder)
{
    if (!builder)
        return;
    free(builder->data);
    free(builder);
}

static int json_builder_ensure(json_stringbuilder *builder, size_t extra)
{
    char *new_data;
    size_t new_size;

    if (!builder)
        return 1;
    if (builder->written_len + extra <= builder->allocated)
        return 0;

    new_size = builder->allocated;
    while (builder->written_len + extra > new_size)
    {
        new_size *= 2;
    }

    new_data = (char *)realloc(builder->data, new_size);
    if (!new_data)
        return 1;
    builder->data = new_data;
    builder->allocated = new_size;
    return 0;
}

static int json_builder_append_bytes(json_stringbuilder *builder, const char *src, size_t len)
{
    if (!builder || !src)
        return 1;
    if (json_builder_ensure(builder, len) != 0)
        return 1;
    memcpy(builder->data + builder->written_len, src, len);
    builder->written_len += len;
    return 0;
}

static int json_builder_append_byte(json_stringbuilder *builder, char value)
{
    if (json_builder_ensure(builder, 1) != 0)
        return 1;
    builder->data[builder->written_len++] = value;
    return 0;
}

static char *json_safestr(const char *str)
{
    size_t len;
    char *out;

    if (!str)
        return NULL;
    len = strlen(str) + 1;
    out = (char *)malloc(len);
    if (!out)
        return NULL;
    memcpy(out, str, len);
    return out;
}

static void json_free_internal(json_data *root, int free_self)
{
    int i;

    if (!root)
        return;

    switch (root->type)
    {
    case JSON_STRING:
        free(root->as.string.data);
        break;
    case JSON_ARRAY:
        if (root->as.array.data)
        {
            for (i = 0; i < root->as.array.len; ++i)
            {
                json_free_internal(&root->as.array.data[i], 0);
            }
            free(root->as.array.data);
        }
        break;
    case JSON_OBJECT:
        if (root->as.object.pairs)
        {
            for (i = 0; i < root->as.object.len; ++i)
            {
                json_kvp *pair = root->as.object.pairs[i];
                if (!pair)
                    continue;
                free(pair->key);
                if (pair->value)
                    json_free_internal(pair->value, 1);
                free(pair);
            }
            free(root->as.object.pairs);
        }
        break;
    case JSON_NUMBER:
    case JSON_BOOLEAN:
    case JSON_NULL:
    default:
        break;
    }

    if (free_self)
        free(root);
}

void json_free(json_data *root)
{
    json_free_internal(root, 1);
}

static int json_append_escaped_string(json_stringbuilder *builder, const char *text)
{
    const unsigned char *cursor;

    if (json_builder_append_byte(builder, '"') != 0)
        return 1;

    for (cursor = (const unsigned char *)(text != NULL ? text : ""); *cursor != '\0'; ++cursor)
    {
        unsigned char value = *cursor;
        switch (value)
        {
        case '"':
            if (json_builder_append_bytes(builder, "\\\"", 2) != 0)
                return 1;
            break;
        case '\\':
            if (json_builder_append_bytes(builder, "\\\\", 2) != 0)
                return 1;
            break;
        case '\b':
            if (json_builder_append_bytes(builder, "\\b", 2) != 0)
                return 1;
            break;
        case '\f':
            if (json_builder_append_bytes(builder, "\\f", 2) != 0)
                return 1;
            break;
        case '\n':
            if (json_builder_append_bytes(builder, "\\n", 2) != 0)
                return 1;
            break;
        case '\r':
            if (json_builder_append_bytes(builder, "\\r", 2) != 0)
                return 1;
            break;
        case '\t':
            if (json_builder_append_bytes(builder, "\\t", 2) != 0)
                return 1;
            break;
        default:
            if (value < 0x20)
            {
                char control[7];
                int written = snprintf(control, sizeof(control), "\\u%04x", value);
                if (written < 0 || json_builder_append_bytes(builder, control, (size_t)written) != 0)
                    return 1;
            }
            else if (json_builder_append_byte(builder, (char)value) != 0)
            {
                return 1;
            }
            break;
        }
    }

    return json_builder_append_byte(builder, '"');
}

static int json_is_effective_integer(double value)
{
    double integer_part;
    if (!isfinite(value))
        return 0;
    return modf(value, &integer_part) == 0.0;
}

static int json_to_string_internal(json_data *root, json_stringbuilder *builder)
{
    int i;

    if (!root || !builder)
        return 1;

    switch (root->type)
    {
    case JSON_STRING:
        return json_append_escaped_string(builder, root->as.string.data);
    case JSON_NUMBER:
    {
        char number[128];
        int written;
        if (json_is_effective_integer(root->as.number.data))
            written = snprintf(number, sizeof(number), "%.0f", root->as.number.data);
        else
            written = snprintf(number, sizeof(number), "%.20g", root->as.number.data);
        if (written < 0)
            return 1;
        return json_builder_append_bytes(builder, number, (size_t)written);
    }
    case JSON_BOOLEAN:
        return json_builder_append_bytes(builder,
                                         root->as.boolean.data ? "true" : "false",
                                         root->as.boolean.data ? 4u : 5u);
    case JSON_NULL:
        return json_builder_append_bytes(builder, "null", 4);
    case JSON_ARRAY:
        if (json_builder_append_byte(builder, '[') != 0)
            return 1;
        for (i = 0; i < root->as.array.len; ++i)
        {
            if (i > 0 && json_builder_append_byte(builder, ',') != 0)
                return 1;
            if (json_to_string_internal(&root->as.array.data[i], builder) != 0)
                return 1;
        }
        return json_builder_append_byte(builder, ']');
    case JSON_OBJECT:
        if (json_builder_append_byte(builder, '{') != 0)
            return 1;
        for (i = 0; i < root->as.object.len; ++i)
        {
            json_kvp *pair = root->as.object.pairs[i];
            if (!pair)
                continue;
            if (i > 0 && json_builder_append_byte(builder, ',') != 0)
                return 1;
            if (json_append_escaped_string(builder, pair->key) != 0)
                return 1;
            if (json_builder_append_byte(builder, ':') != 0)
                return 1;
            if (json_to_string_internal(pair->value, builder) != 0)
                return 1;
        }
        return json_builder_append_byte(builder, '}');
    default:
        return 1;
    }
}

char *json_tostring(json_data *root)
{
    char *out;
    json_stringbuilder *builder = json_builder_init();
    if (!builder)
        return NULL;
    if (json_to_string_internal(root, builder) != 0)
    {
        json_builder_free(builder);
        return NULL;
    }
    out = (char *)malloc(builder->written_len + 1);
    if (!out)
    {
        json_builder_free(builder);
        return NULL;
    }
    memcpy(out, builder->data, builder->written_len);
    out[builder->written_len] = '\0';
    json_builder_free(builder);
    return out;
}

json_data *init_json_object_populated(int count, ...)
{
    int i;
    va_list args;
    json_data *data;

    if (count < 0)
        return NULL;

    data = (json_data *)malloc(sizeof(json_data));
    if (!data)
        return NULL;
    data->type = JSON_OBJECT;
    data->as.object.len = count;
    data->as.object.pairs = count > 0 ? (json_kvp **)malloc(sizeof(json_kvp *) * (size_t)count) : NULL;
    if (count > 0 && !data->as.object.pairs)
    {
        free(data);
        return NULL;
    }

    va_start(args, count);
    for (i = 0; i < count; ++i)
    {
        data->as.object.pairs[i] = va_arg(args, json_kvp *);
    }
    va_end(args);
    return data;
}

json_data *init_json_object(void)
{
    json_data *data = (json_data *)malloc(sizeof(json_data));
    if (!data)
        return NULL;
    data->type = JSON_OBJECT;
    data->as.object.len = 0;
    data->as.object.pairs = NULL;
    return data;
}

json_data *init_json_string(const char *str)
{
    json_data *data = (json_data *)malloc(sizeof(json_data));
    if (!data)
        return NULL;
    data->type = JSON_STRING;
    data->as.string.data = json_safestr(str != NULL ? str : "");
    if (!data->as.string.data)
    {
        free(data);
        return NULL;
    }
    return data;
}

json_data *init_json_array(int count, ...)
{
    int i;
    va_list args;
    json_data *data;

    if (count < 0)
        return NULL;

    data = (json_data *)malloc(sizeof(json_data));
    if (!data)
        return NULL;
    data->type = JSON_ARRAY;
    data->as.array.len = count;
    data->as.array.data = count > 0 ? (json_data *)malloc(sizeof(json_data) * (size_t)count) : NULL;
    if (count > 0 && !data->as.array.data)
    {
        free(data);
        return NULL;
    }

    va_start(args, count);
    for (i = 0; i < count; ++i)
    {
        json_data *element = va_arg(args, json_data *);
        data->as.array.data[i] = *element;
    }
    va_end(args);
    return data;
}

json_data *init_json_number(double num)
{
    json_data *data = (json_data *)malloc(sizeof(json_data));
    if (!data)
        return NULL;
    data->type = JSON_NUMBER;
    data->as.number.data = num;
    return data;
}

json_data *init_json_boolean(bool boolean)
{
    json_data *data = (json_data *)malloc(sizeof(json_data));
    if (!data)
        return NULL;
    data->type = JSON_BOOLEAN;
    data->as.boolean.data = boolean;
    return data;
}

json_data *init_json_null(void)
{
    json_data *data = (json_data *)malloc(sizeof(json_data));
    if (!data)
        return NULL;
    data->type = JSON_NULL;
    return data;
}

json_kvp *init_kvp(const char *key, json_data *value)
{
    json_kvp *pair;

    if (!key || !value)
        return NULL;

    pair = (json_kvp *)malloc(sizeof(json_kvp));
    if (!pair)
        return NULL;
    pair->key = json_safestr(key);
    if (!pair->key)
    {
        free(pair);
        return NULL;
    }
    pair->value = value;
    return pair;
}

void json_push(json_data *obj, json_kvp *kvp)
{
    json_kvp **pairs;

    if (!obj || obj->type != JSON_OBJECT || !kvp)
        return;

    obj->as.object.len++;
    if (!obj->as.object.pairs)
        pairs = (json_kvp **)malloc(sizeof(json_kvp *));
    else
        pairs = (json_kvp **)realloc(obj->as.object.pairs, sizeof(json_kvp *) * (size_t)obj->as.object.len);

    if (!pairs)
    {
        obj->as.object.len--;
        return;
    }

    obj->as.object.pairs = pairs;
    obj->as.object.pairs[obj->as.object.len - 1] = kvp;
}

void json_push_arr(json_data *arr, json_data *data)
{
    int new_len;
    json_data *new_items;

    if (!arr || arr->type != JSON_ARRAY || !data)
        return;

    new_len = arr->as.array.len + 1;
    if (!arr->as.array.data)
        new_items = (json_data *)malloc(sizeof(json_data));
    else
        new_items = (json_data *)realloc(arr->as.array.data, sizeof(json_data) * (size_t)new_len);

    if (!new_items)
        return;

    arr->as.array.data = new_items;
    arr->as.array.data[new_len - 1] = *data;
    arr->as.array.len = new_len;
}

json_data *get_value(json_data *obj, const char *key)
{
    int i;
    if (!obj || obj->type != JSON_OBJECT || !key)
        return NULL;
    for (i = 0; i < obj->as.object.len; ++i)
    {
        json_kvp *pair = obj->as.object.pairs[i];
        if (pair && strcmp(pair->key, key) == 0)
            return pair->value;
    }
    return NULL;
}

int json_object_add(json_data *obj, const char *key, json_data *value)
{
    json_kvp *pair;
    if (!obj || obj->type != JSON_OBJECT || !key || !value)
        return 1;
    pair = init_kvp(key, value);
    if (!pair)
    {
        json_free(value);
        return 1;
    }
    json_push(obj, pair);
    return 0;
}

int json_object_add_string(json_data *obj, const char *key, const char *value)
{
    return json_object_add(obj, key, init_json_string(value != NULL ? value : ""));
}

int json_object_add_number(json_data *obj, const char *key, double value)
{
    return json_object_add(obj, key, init_json_number(value));
}

int json_object_add_boolean(json_data *obj, const char *key, bool value)
{
    return json_object_add(obj, key, init_json_boolean(value));
}

int json_object_add_null(json_data *obj, const char *key)
{
    return json_object_add(obj, key, init_json_null());
}

int json_object_add_object(json_data *obj, const char *key, json_data *value)
{
    return json_object_add(obj, key, value);
}

int json_object_add_array(json_data *obj, const char *key, json_data *value)
{
    return json_object_add(obj, key, value);
}

int json_array_add(json_data *arr, json_data *value)
{
    if (!arr || arr->type != JSON_ARRAY || !value)
        return 1;
    json_push_arr(arr, value);
    return 0;
}

int json_array_add_string(json_data *arr, const char *value)
{
    return json_array_add(arr, init_json_string(value != NULL ? value : ""));
}

int json_array_add_number(json_data *arr, double value)
{
    return json_array_add(arr, init_json_number(value));
}

int json_array_add_boolean(json_data *arr, bool value)
{
    return json_array_add(arr, init_json_boolean(value));
}

int json_array_add_null(json_data *arr)
{
    return json_array_add(arr, init_json_null());
}

int json_array_add_object(json_data *arr, json_data *value)
{
    return json_array_add(arr, value);
}

int json_array_add_array(json_data *arr, json_data *value)
{
    return json_array_add(arr, value);
}
