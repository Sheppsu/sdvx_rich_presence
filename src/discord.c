#include "discord.h"
#include "json.h"
#include <stdio.h>
#include <stdbool.h>
#include <windows.h>

HANDLE ipcHandle;
void (*dispatchCallback)() = NULL;

// IO

bool open_connection() {
    ipcHandle = CreateFile(IPC_PATH, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (ipcHandle == INVALID_HANDLE_VALUE) {
        printf("CreateFile failed with error %d\n", GetLastError());
        return false;
    }
    return true;
}

bool peek_pipe(DWORD* bytesAvailable) {
    if (!PeekNamedPipe(ipcHandle, NULL, 0, NULL, bytesAvailable, NULL)) {
        printf("PeekNamedPipe failed with error code %d", GetLastError());
        return false;
    }
    return true;
}

bool read_file(char* buf, DWORD readSize) {
    if (!ReadFile(ipcHandle, buf, readSize, NULL, NULL)) {
        printf("ReadFile failed with error code %d", GetLastError());
        return false;
    }
    return true;
}

DWORD write_file(void* data, DWORD dataSize) {
    DWORD dataWritten;
    if (!WriteFile(ipcHandle, data, dataSize, &dataWritten, NULL)) {
        printf("WriteFile failed with error %d", GetLastError());
        return 0;
    }

    return dataWritten;
}

// TRANSLATION

char* get_cmd_str(unsigned char cmd) {
    switch(cmd) {
        case CMD_DISPATCH:
            return "DISPATCH";
        case CMD_AUTHORIZE:
            return "AUTHORIZE";
        case CMD_AUTHENTICATE:
            return "AUTHENTICATE";
        case CMD_GET_GUILD:
            return "GET_GUILD";
        case CMD_GET_GUILDS:
            return "GET_GUILDS";
        case CMD_GET_CHANNEL:
            return "GET_CHANNEL";
        case CMD_SUBSCRIBE:
            return "SUBSCRIBE";
        case CMD_UNSUBSCRIBE:
            return "UNSUBSCRIBE";
        case CMD_SET_USER_VOICE_SETTINGS:
            return "SET_USER_VOICE_SETTINGS";
        case CMD_SELECT_VOICE_CHANNEL:
            return "SELECT_VOICE_CHANNEL";
        case CMD_GET_SELECTED_VOICE_CHANNEL:
            return "GET_SELECTED_VOICE_CHANNEL";
        case CMD_SELECT_TEXT_CHANNEL:
            return "SELECT_TEXT_CHANNEL";
        case CMD_GET_VOICE_SETTINGS:
            return "GET_VOICE_SETTINGS";
        case CMD_SET_VOICE_SETTINGS:
            return "SET_VOICE_SETTINGS";
        case CMD_SET_CERTIFIED_DEVICES:
            return "SET_CERTIFIED_DEVICES";
        case CMD_SET_ACTIVITY:
            return "SET_ACTIVITY";
        case CMD_SEND_ACTIVITY_JOIN_INVITE:
            return "SEND_ACTIVITY_JOIN_INVITE";
        case CMD_CLOSE_ACTIVITY_REQUEST:
            return "CLOSE_ACTIVITY_REQUEST";
    }
}

void generate_nonce(char* nonce) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i=0; i<16; i++) {
        nonce[i] = charset[rand() % (sizeof charset - 1)];
    }
    nonce[16] = '\0';
}

// UTIL

bool send_json(unsigned char opcode, char* json) {
    char data[1024];
    unsigned long jsonLen = strlen(json);

    data[0] = opcode;
    memset(data+1, 0, 3);
    memcpy(data+4, &jsonLen, sizeof(unsigned long));
    strcpy(data+8, json);

    if (write_file(data, jsonLen+8) == 0) return false;
    return true;
}

bool handshake(char* clientId) {
    char json[64];
    sprintf(json, "{\"v\": 1, \"client_id\": \"%s\"}", clientId);
    if (!send_json(OC_HANDSHAKE, json)) return false;
    return true;
}

void handle_event(int opcode, JSON_DICT jsonDict) {
    for (int i=0; i<jsonDict.numItems; i++) {
        JSON_DICT_PAIR pair = jsonDict.items[i];
        if (strcmp(pair.key, "cmd") == 0 && strcmp(pair.value.value, get_cmd_str(CMD_DISPATCH)) == 0) {
            (*dispatchCallback)();
            return;
        }
    }
    // TODO: callback based on nonce
}

// MAIN

bool discord_connect(char* clientId, void (*callback)()) {
    dispatchCallback = callback;
    printf("Opening discord connection\n");
    if (!open_connection()) return false;
    printf("Doing discord handshake\n");
    if (!handshake(clientId)) return false;
    return true;
}

bool discord_update() {
    DWORD bytesAvailable = 0;
    if (!peek_pipe(&bytesAvailable)) return false;
    if (bytesAvailable == 0) return true;

    char* buf = malloc(bytesAvailable+1);
    read_file(buf, bytesAvailable);

    int size = 0;
    while (size < bytesAvailable) {
        int opcode = *(int*)(buf+size);
        int length = *((int*)(buf+size+4));
        JSON_DICT jsonDict;
        int jsonLength;

        // char printJson[1024];
        // memcpy(printJson, buf+size+8, length);
        // printJson[length] = '\0';

        if (!json_dict_from_string(buf+size+8, &jsonDict, &jsonLength)) return false;
        handle_event(opcode, jsonDict);
        json_free_dict(&jsonDict);
        size += (8+length);
    }

    free(buf);
    return true;
}

bool discord_set_activity(DWORD pid, char* activityJson) {
    char json[1024];
    char nonce[17];
    generate_nonce(nonce);
    sprintf(json, "{\"cmd\": \"%s\", \"args\": {\"pid\": %d, \"activity\": %s}, \"nonce\": \"%s\"}", get_cmd_str(CMD_SET_ACTIVITY), pid, activityJson, nonce);
    if (!send_json(OC_FRAME, json)) return false;
    return true;
}
