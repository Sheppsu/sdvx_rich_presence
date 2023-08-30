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
    for (int i=0; i<26; i++) {
        UI_OBJECT obj = MemoryData.UiObjects[i];
        if (strcmp(obj.label, label) == 0) {
            return MemoryData.UiObjects[i].text;
        }
    }
    return NULL;
}

void get_details(char* details) {
    char* artist = get_ui_value("info_usr/artist_selected_usr");
    char* title = get_ui_value("info_usr/title_selected_usr");
    switch(MemoryData.GameState) {
        case STATE_MUSIC_SELECT:
            strcpy(details, "Song selection");
            break;
        case STATE_STAGE:
            if (artist == NULL || title == NULL) {
                strcpy(details, "Currently playing ...");
            } else {
                sprintf(
                    details, 
                    "Currently playing %s - %s", 
                    artist, 
                    title
                );
            }
            break;
        case STATE_RESULTS:
            if (artist == NULL || title == NULL) {
                strcpy(details, "Results screen ...");
            } else {
                sprintf(
                    details, 
                    "Results screen: %s - %s", 
                    artist, 
                    title
                );
            }
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
        default:
            strcpy(details, "");
    }
}

int main(int argc, char **argv) {
    if (!memory_reader_init()) return 1;
    if (!discord_connect("1032756213445836801", on_dispatch)) return 1;

    DWORD pid = memory_reader_process_id();
    unsigned long long createdAt = time(NULL);
    unsigned long long tsStart = time(NULL);
    char details[1024];

    char activityJson[1024];
    while (discord_update()) {
        Sleep(100);
        if (!memory_reader_update()) return 1;
        if (!isConnected) continue;
        get_details(details);
        sprintf(
            activityJson, 
            "{\"type\": 0, \"timestamps\": {\"start\": %d}, \"details\": \"%s\", \"assets\": {\"large_image\": \"sdvx\"}}",
            tsStart,
            details
        );
        if (!discord_set_activity(pid, activityJson)) return 1;
    }
    if (!memory_reader_cleanup()) return 1;
    return 0;
}
