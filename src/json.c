#include "json.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

// TODO: null and boolean types currently fall under int and just get parsed to 0

long long* parse_int_value(char* jsonString, int* jsonLength) {
    int length = 0;
    while (jsonString[length] != ',' && jsonString[length] != '}' && jsonString[length] != ']') {
        length++;
    }

    if (length > 19) {
        return NULL;
    }

    char numStr[20];
    memcpy(numStr, jsonString, length);
    numStr[length] = '\0';
    *jsonLength = length;

    long long* value = malloc(sizeof(long long));
    *value = atoll(numStr);
    return value;
}

char* parse_string_value(char* jsonString, int* jsonLength) {
    jsonString++;
    int length = 0;
    int i = 0;
    bool ignore = false;
    while (jsonString[i] != '\"' && !ignore) {
        if (jsonString[i] == '\\' && !ignore) {
            ignore = true;
        } else {
            ignore = false;
            length++;
        }
        i++;
    }
    *jsonLength = i+2;

    char* value = malloc(length+1);
    value[length] = '\0';
    i = 0;
    length = 0;
    while (jsonString[i] != '\"' && !ignore) {
        if (jsonString[i] == '\\' && !ignore) {
            ignore = true;
        } else {
            ignore = false;
            value[length] = jsonString[i];
            length++;
        }
        i++;
    }

    return value;
}

void* json_parse_value(char* jsonString, unsigned char* valueType, int* jsonLength) {
    switch(jsonString[0]) {
        case '{':
            *valueType = JSON_VT_DICT;
            JSON_DICT* jsonDict = malloc(sizeof(JSON_DICT));
            json_dict_from_string(jsonString, jsonDict, jsonLength);
            return jsonDict;
        case '[':
            *valueType = JSON_VT_LIST;
            JSON_LIST* jsonList = malloc(sizeof(JSON_LIST));
            json_list_from_string(jsonString, jsonList, jsonLength);
            return jsonList;
        case '\"':
            *valueType = JSON_VT_STR;
            return parse_string_value(jsonString, jsonLength);
        default:
            if (memcmp(jsonString, "null", 4) == 0) {
                *valueType = JSON_VT_NULL;
                *jsonLength = 4;
                return NULL;
            } else if (memcmp(jsonString, "false", 5) == 0) {
                *valueType = JSON_VT_BOOLEAN;
                *jsonLength = 5;
                return &JSON_FALSE_VALUE;
            } else if (memcmp(jsonString, "true", 4) == 0) {
                *valueType = JSON_VT_BOOLEAN;
                *jsonLength = 4;
                return &JSON_TRUE_VALUE;
            } else {
                *valueType = JSON_VT_INT;
                return parse_int_value(jsonString, jsonLength);
            }
           
    }
}

/*
    Basic json parser which assumes the json is valid and in a minimal format
*/
bool json_dict_from_string(char* jsonString, JSON_DICT* jsonDict, int* jsonLength) {
    size_t jsonLen = strlen(jsonString);
    if (jsonLen < 2) {
        printf("Provided json length is less than 2\n");
        return false;
    }

    if (jsonString[0] != '{') {
        printf("Provided json dict string does not start with {\n");
        return false;
    }

    int depthCounter = 0;
    int numPairs = 0;
    int length = 0;
    for (int i=0; depthCounter!=0||i==0; i++) {
        if (depthCounter == 1 && jsonString[i] == ':') {
            numPairs++;
        }
        else if (jsonString[i] == '{' || jsonString[i] == '[') {
            depthCounter++;
        }
        else if (jsonString[i] == '}' || jsonString[i] == ']') {
            depthCounter--;
        }
        length++;
    }
    *jsonLength = length;

    if (numPairs == 0) {
        jsonDict = NULL;
        return true;
    }

    JSON_DICT_PAIR* pairs = calloc(numPairs, sizeof(JSON_DICT_PAIR));
    jsonDict->numItems = numPairs;
    jsonDict->items = pairs;
    int jsonI = 1;
    for (int i=0; i<numPairs; i++) {
        int jsonLength2;
        pairs[i].key = parse_string_value(jsonString+jsonI, &jsonLength2);
        jsonI += jsonLength2+1;

        unsigned char valueType;
        void* value = json_parse_value(jsonString+jsonI, &valueType, &jsonLength2);
        jsonI += jsonLength2+1;
        JSON_VALUE jsonValue = {value, valueType};
        pairs[i].value = jsonValue;
    }

    return true;
}

bool json_list_from_string(char* jsonString, JSON_LIST* jsonList, int* jsonLength) {
    size_t jsonLen = strlen(jsonString);
    if (jsonLen < 2) {
        printf("Provided json length is less than 2\n");
        return false;
    }

    if (jsonString[0] != '[') {
        printf("Provided json list string does not start with [\n");
        return false;
    }

    int depthCounter = 0;
    int numItems = 0;
    int length = 0;
    for (int i=0; depthCounter!=0||i==0; i++) {
        if (depthCounter == 1 && jsonString[i] == ',') {
            numItems++;
        }
        else if (jsonString[i] == '{' || jsonString[i] == '[') {
            depthCounter++;
        }
        else if (jsonString[i] == '}' || jsonString[i] == ']') {
            depthCounter--;
        }
        length++;
    }
    *jsonLength = length;
    if (length > 2) {
        numItems++;
    }

    JSON_VALUE* items = calloc(numItems, sizeof(JSON_VALUE));
    int jsonI = 1;
    for (int i=0; i<numItems; i++) {
        int jsonLength2;
        unsigned char valueType;
        void* value = json_parse_value(jsonString+jsonI, &valueType, &jsonLength2);

        JSON_VALUE jsonValue = {value, valueType};
        items[i] = jsonValue;
    }
    
    return true;
}

void json_free_value(JSON_VALUE value) {
    if (value.valueType == JSON_VT_NULL || value.valueType == JSON_VT_BOOLEAN) return;
    if (value.valueType == JSON_VT_DICT) {
        json_free_dict(value.value);
        return free(value.value);
    };
    if (value.valueType == JSON_VT_LIST) {
        json_free_list(value.value);
        return free(value.value);
    }
    free(value.value);
}

void json_free_dict(JSON_DICT* jsonDict) {
    for (int i=0; i<jsonDict->numItems; i++) {
        JSON_DICT_PAIR pair = jsonDict->items[i];
        free(pair.key);
        json_free_value(pair.value);
    }
    free(jsonDict->items);
}

void json_free_list(JSON_LIST* jsonList) {
    for (int i=0; i<jsonList->numItems; i++) {
        JSON_VALUE value = jsonList->items[i];
        json_free_value(value);
    }
    free(jsonList->items);
}
