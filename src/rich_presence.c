#include "sdvx_memory_reader/src/memory_reader.h"
#include "discord.h"
#include "cJSON.h"
#include <stdio.h>
#include <time.h>

#define PROGRAM_VERSION "v2.1"

bool isConnected = false;

void on_dispatch() {
    isConnected = true;
    printf("Handshake complete\nIf you see a \"Memory read failed with error code 299\" but the program hasn't stopped, you can just ignore it. It may happen in between transitioning states.\n");
}

void get_details(char* details, char* state) {
    bool isInSongState = false;

    state[0] = '\0';
    details[0] = '\0';

    // Get presence info by state
    switch(MemoryData.gameState) {
        case MUSICSELECT:
            strcpy(details, "Song selection");
            break;
        case ALTERNATIVE_GAME_SCENE:
            strcpy(details, "Currently playing");
            isInSongState = true;
            break;
        case RESULT_SCENE:
            strcpy(details, "Results screen");
            isInSongState = true;
            break;
        case TITLE_SCENE:
            strcpy(details, "Title screen");
            break;
        case DEMO_GAME:
            strcpy(details, "Title screen");
            break;
        case DEMO_TUTORIAL:
            strcpy(details, "Tutorial screen");
            break;
        case TUTORIAL:
            strcpy(details, "Tutorial screen");
            break;
        case BOOT:
            strcpy(details, "Boot screen");
            break;
		default: {
            char* name = MemoryData.gameStateName;
            size_t nameLen = strlen(name);
            details[nameLen] = '\0';

            bool caps = true;
            for (int i=0; i<nameLen; i++) {
                // captialize letter after space + replace underscores with spaces
                if (name[i] == '_' || name[i] == ' ') {
                    details[i] = ' ';
                    caps = true;
                    continue;
                }

                if (caps) {
                    details[i] = name[i];
                    caps = false;
                } else {
                    details[i] = tolower(name[i]);
                }
            }
        }
    }


    if (isInSongState) {
        MUSIC_ENTRY* entry = memory_reader_get_currently_playing();
        if (entry != NULL) {
            sprintf(state, "%s - %s [Level %d]", entry->artistName, entry->titleName, entry->level[MemoryData.currentlyPlayingDifficulty]+1);
        }
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

    while (discord_update()) {
        Sleep(100);
        if (!memory_reader_update()) return 1;
        if (!isConnected) continue;
        get_details(details, state);

        cJSON* activity = cJSON_CreateObject();
        cJSON_AddNumberToObject(activity, "type", 0);
        cJSON* timestamp = cJSON_AddObjectToObject(activity, "timestamp");
        cJSON_AddNumberToObject(timestamp, "start", (double)tsStart);
        cJSON_AddStringToObject(activity, "details", details);
        if (strlen(state) != 0) {
            cJSON_AddStringToObject(activity, "state", state);
        }
        cJSON* assets = cJSON_AddObjectToObject(activity, "assets");
        cJSON_AddStringToObject(assets, "large_image", "sdvx");

        if (!discord_set_activity(pid, activity)) return 1;
    }
    if (!memory_reader_cleanup()) return 1;
    return 0;
}

int main(int argc, char **argv) {
    printf("Running version %s\n", PROGRAM_VERSION);

    attach_svdx();
    attach_discord();
    
    return do_discord_update_loop();
}
