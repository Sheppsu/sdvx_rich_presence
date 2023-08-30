#ifndef DISCORD_H
#define DISCORD_H

#include <windows.h>
#include <stdbool.h>

// Constants

static const char* IPC_PATH = "\\\\?\\pipe\\discord-ipc-0";

enum Commands {
    CMD_DISPATCH,
    CMD_AUTHORIZE,
    CMD_AUTHENTICATE,
    CMD_GET_GUILD,
    CMD_GET_GUILDS,
    CMD_GET_CHANNEL,
    CMD_SUBSCRIBE,
    CMD_UNSUBSCRIBE,
    CMD_SET_USER_VOICE_SETTINGS,
    CMD_SELECT_VOICE_CHANNEL,
    CMD_GET_SELECTED_VOICE_CHANNEL,
    CMD_SELECT_TEXT_CHANNEL,
    CMD_GET_VOICE_SETTINGS,
    CMD_SET_VOICE_SETTINGS,
    CMD_SET_CERTIFIED_DEVICES,
    CMD_SET_ACTIVITY,
    CMD_SEND_ACTIVITY_JOIN_INVITE,
    CMD_CLOSE_ACTIVITY_REQUEST
};

enum OPCODES {
    OC_HANDSHAKE = 0,
    OC_FRAME = 1,
    OC_CLOSE = 2,
    OC_PING = 3,
    OC_PONG = 4
};

enum ACTIVITY_TYPES {
    AT_GAME = 0,
    AT_STREAMING = 1,
    AT_LISTENING = 2,
    AT_WATCHING = 3,
    AT_CUSTOM = 4,
    AT_COMPETING = 5
};

// stuff

extern HANDLE ipcHandle;

bool discord_connect(char* clientId, void (*callback)());
bool discord_update();
bool discord_set_activity(DWORD pid, char* activityJson);

#endif /* DISCORD_H */