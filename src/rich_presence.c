#include "sdvx_memory_reader/src/memory_reader.h"
#include "discord.h"
#include <stdio.h>
#include <time.h>

bool isConnected = false;

void on_dispatch() {
    isConnected = true;
    printf("Handshake complete\nIf you see a \"Memory read failed with error code 299\" but the program hasn't stopped, you can just ignore it. Happens in between transitioning states.\n");
}

char* get_ui_value(char* label) {
    for (int i=0; i<MemoryData.uiObjCount; i++) {
        UI_OBJECT obj = MemoryData.uiObjects[i];
        if (strcmp(obj.label, label) == 0) {
            return MemoryData.uiObjects[i].text;
        }
    }
    return NULL;
}

void get_details(char* details, char* state) {
    char* artist = get_ui_value("info_usr/artist_selected_usr");
    char* title = get_ui_value("info_usr/title_selected_usr");
    bool songInfoIsAvailable = artist != NULL && title != NULL;
    bool isInSongState = false;

    state[0] = '\0';
    details[0] = '\0';

    // Get presence info by state
    switch(MemoryData.gameState) {
        case STATE_MUSIC_SELECT:
            strcpy(details, "Song selection");
            break;
        case STATE_STAGE:
            strcpy(details, "Currently playing");
            isInSongState = true;
            break;
        case STATE_RESULTS:
            strcpy(details, "Results screen");
            isInSongState = true;
            break;
        case STATE_TITLE:
            strcpy(details, "Title screen");
            break;
        case STATE_COURSE_SELECT:
            strcpy(details, "Course select");
            break;
        case STATE_COURSE_RESULT:
            strcpy(details, "Course result");
            break;
        case STATE_MODE_SELECT:
            strcpy(details, "Mode select");
            break;
        case STATE_MENU_SELECT:
            strcpy(details, "Menu select");
            break;
        case STATE_ENTRY:
            strcpy(details, "Entry screen");
            break;
        case STATE_STARTUP:
            strcpy(details, "Startup screen");
            break;
        case STATE_LOADING:
            strcpy(details, "Loading screen");
            break;
    }

    if (isInSongState && songInfoIsAvailable) {
        sprintf(state, "%s - %s", artist, title);
    }
}


static void print_seperator() {
    printf("--------------------------\n");
}

static void attach_svdx() {
    while (!memory_reader_init()) {
        Sleep(1000);
        print_seperator();
    }
}

static void getDiscordClientId(char * discordClientId) {
    char *envVarName = "SDVX_DISCORD_RPC_CLIENT_ID";
    
    char *envVarValue = getenv(envVarName);

    if (envVarValue) {
        strcpy(discordClientId, envVarValue);
    }
}

static void attach_discord() {
    char discordClientId[20] = "1032756213445836801";
    getDiscordClientId(discordClientId);

    while (!discord_connect(discordClientId, on_dispatch)) {
        Sleep(1000);
        print_seperator();
    }
}

static int do_discord_update_loop() {
    DWORD pid = memory_reader_process_id();
    unsigned long long createdAt = time(NULL);
    unsigned long long tsStart = time(NULL);
    char details[1024];
    char state[1024];

    char activityJson[1024];
    while (discord_update()) {
        Sleep(100);
        if (!memory_reader_update()) return 1;
        if (!isConnected) continue;
        get_details(details, state);

        if (strlen(state) != 0) {
            sprintf(
                activityJson, 
                "{\"type\": 0, \"timestamps\": {\"start\": %d}, \"details\": \"%s\", \"state\": \"%s\", \"assets\": {\"large_image\": \"sdvx\"}}",
                tsStart,
                details,
                state
            );
        } else {
            sprintf(
                activityJson, 
                "{\"type\": 0, \"timestamps\": {\"start\": %d}, \"details\": \"%s\", \"assets\": {\"large_image\": \"sdvx\"}}",
                tsStart,
                details
            );
        }
        
        if (!discord_set_activity(pid, activityJson)) return 1;
    }
    if (!memory_reader_cleanup()) return 1;
    return 0;
}

int main(int argc, char **argv) {
    attach_svdx();
    attach_discord();
    
    return do_discord_update_loop();
}
