#ifndef JSON_H
#define JSON_H

#include <stdbool.h>

static bool JSON_FALSE_VALUE = false;
static bool JSON_TRUE_VALUE = true;

enum JSON_VALUE_TYPE {
    JSON_VT_INT,
    JSON_VT_STR,
    JSON_VT_BOOLEAN,
    JSON_VT_NULL,
    JSON_VT_DICT,
    JSON_VT_LIST
};

typedef struct _JSON_VALUE {
    void* value;
    unsigned char valueType;
} JSON_VALUE;

typedef struct _JSON_DICT_PAIR {
    char* key;
    JSON_VALUE value;
} JSON_DICT_PAIR;

typedef struct _JSON_DICT {
    JSON_DICT_PAIR* items;
    int numItems;
} JSON_DICT;

typedef struct _JSON_LIST {
    JSON_VALUE* items;
    int numItems;
} JSON_LIST;

bool json_dict_from_string(char* jsonString, JSON_DICT* jsonDict, int* jsonLength);
bool json_list_from_string(char* jsonString, JSON_LIST* jsonList, int* jsonLength);
void* json_parse_value(char* jsonString, unsigned char* valueType, int* jsonLength);
void json_free_value(JSON_VALUE value);
void json_free_dict(JSON_DICT* jsonDict);
void json_free_list(JSON_LIST* jsonList);

#endif /* JSON_H */