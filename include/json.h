#ifndef JSON_H
#define JSON_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum jsondatatype
{
    JSON_STRING,
    JSON_NUMBER,
    JSON_ARRAY,
    JSON_OBJECT,
    JSON_BOOLEAN,
    JSON_NULL
} jsondatatype;

typedef struct json_data json_data;

typedef struct json_kvp
{
    char *key;
    json_data *value;
} json_kvp;

typedef struct json_object
{
    json_kvp **pairs;
    int len;
} json_object;

typedef struct json_string
{
    char *data;
} json_string;

typedef struct json_number
{
    double data;
} json_number;

typedef struct json_bool
{
    bool data;
} json_bool;

typedef struct json_array
{
    json_data *data;
    int len;
} json_array;

struct json_data
{
    jsondatatype type;
    union
    {
        json_string string;
        json_number number;
        json_bool boolean;
        json_array array;
        json_object object;
    } as;
};

char *json_tostring(json_data *root);
void json_free(json_data *root);
json_kvp *init_kvp(const char *key, json_data *value);
void json_push(json_data *obj, json_kvp *kvp);

json_data *init_json_object(void);
json_data *init_json_string(const char *str);
json_data *init_json_array(int count, ...);
json_data *init_json_number(double num);
json_data *init_json_boolean(bool boolean);
json_data *init_json_null(void);
json_data *init_json_object_populated(int count, ...);
json_data *get_value(json_data *obj, const char *key);
void json_push_arr(json_data *arr, json_data *data);

int json_object_add(json_data *obj, const char *key, json_data *value);
int json_object_add_string(json_data *obj, const char *key, const char *value);
int json_object_add_number(json_data *obj, const char *key, double value);
int json_object_add_boolean(json_data *obj, const char *key, bool value);
int json_object_add_null(json_data *obj, const char *key);
int json_object_add_object(json_data *obj, const char *key, json_data *value);
int json_object_add_array(json_data *obj, const char *key, json_data *value);

int json_array_add(json_data *arr, json_data *value);
int json_array_add_string(json_data *arr, const char *value);
int json_array_add_number(json_data *arr, double value);
int json_array_add_boolean(json_data *arr, bool value);
int json_array_add_null(json_data *arr);
int json_array_add_object(json_data *arr, json_data *value);
int json_array_add_array(json_data *arr, json_data *value);

#ifdef __cplusplus
}
#endif

#endif
