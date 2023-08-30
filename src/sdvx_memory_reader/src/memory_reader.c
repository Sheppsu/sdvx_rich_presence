#include "memory_reader.h"
#include "decode.h"
#include <tlhelp32.h>
#include <Psapi.h>
#include <stdio.h>
#include <Shlwapi.h>


DWORD currentProcessId = 0;
HANDLE currentProcessHandle;
MODULEINFO currentProcessInfo;
MODULEINFO avs2CoreInfo;

SEARCH_PATTERN searchPatterns[2] = {
    {PATTERN_GAME_STATE, 8, 0, 0, 0x0, NULL},
    {NULL, 0, 0x146348, 1, 0x1, NULL}
};
unsigned char patternCount = 2;

MEMORY_DATA MemoryData;


/* 
    Get module information from a module handle
*/
bool get_module_info(HMODULE moduleHandle, MODULEINFO* output) {
    if (GetModuleInformation(currentProcessHandle, moduleHandle, output, sizeof(MODULEINFO)) == 0) {
        printf("Failed to get module information with error code %d\n", GetLastError());
        return false;
    }
    return true;
}

/*
    Get needed module info (for sv6c.exe and avs2-core.dll)
*/
bool init_module_info() {
    if (currentProcessHandle == NULL) {
        printf("Attempted to get module info before opening process\n");
        return false;
    }

    DWORD lpcbNeeded;
    if (EnumProcessModules(currentProcessHandle, NULL, 0, &lpcbNeeded) == 0) {
        printf("First EnumProcessModules call failed with error code %d\n", GetLastError());
        return false;
    }
    HMODULE* modules = malloc(lpcbNeeded);
    if (EnumProcessModules(currentProcessHandle, modules, lpcbNeeded, &lpcbNeeded) == 0) {
        printf("Second EnumProcessModules call failed with error code %d\n", GetLastError());
        return false;
    }

    DWORD moduleCount = lpcbNeeded / sizeof(HMODULE);
    for (int i=0; i<moduleCount; i++) {
        char modFilePath[MAX_PATH];
        if (GetModuleFileNameExA(currentProcessHandle, modules[i], modFilePath, MAX_PATH) == 0) {
            printf("Failed to get module file name with error code %d\n", GetLastError());
            return false;
        }
        PathStripPathA(modFilePath);

        if (strcmp(modFilePath, "sv6c.exe") == 0) {
            if (!get_module_info(modules[i], &currentProcessInfo)) return false;
        } else if (strcmp(modFilePath, "avs2-core.dll") == 0) {
            if (!get_module_info(modules[i], &avs2CoreInfo)) return false;
        }

        if (currentProcessInfo.lpBaseOfDll != NULL && avs2CoreInfo.lpBaseOfDll != NULL) return true;
    }

    printf("Unable to get module info for sv6c.exe and avs2-core.dll");
    return false;
}

/*
    Sets currentProcessId if it succeeds in finding the process.
    Returns false upon failing to do so.
*/
bool set_current_process(char* processName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        printf("CreateToolhelp32Snapshot failed with error code %d\n", GetLastError());
        return false;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    BOOL hResult = Process32First(hSnapshot, &pe);
    while (hResult) {
        if (strcmp(processName, pe.szExeFile) == 0) {
            currentProcessId = pe.th32ProcessID;
            break;
        }
        hResult = Process32Next(hSnapshot, &pe);
    }

    CloseHandle(hSnapshot);

    if (currentProcessId == 0) {
        printf("Failed to find process named \"%s\"\n", processName);
        return false;
    }
    return true;
}

/*
    Sets currentProcessHandle if currentProcessid is set and OpenProcess succeeds.
    Calls GetProcessInformation to set currentProcessInfo.
    Returns false upon failing to do so.
*/
bool open_process() {
    if (currentProcessId == 0) {
        printf("Attempted to open a process before setting the current process id\n");
        return false;
    }

    currentProcessHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, 0, currentProcessId);
    if (currentProcessHandle == NULL) {
        printf("Process opening failed with error code %d\n", GetLastError());
        return false;
    }
    
    return true;
}

/*
    Closes a process which was opened with open_process.
    Returns false upon failing to do so.
*/
bool close_process() {
    if (currentProcessHandle == NULL) {
        printf("Attempted to close a process which is not open\n");
        return false;
    }

    if (!CloseHandle(currentProcessHandle)) {
        printf("Process handle closing failed with error code %d\n", GetLastError());
        return false;
    }

    return true;
}

/*
    Gets information of an address page.
    Returns false when any error other than 87 occurs.
*/
bool query_page(MEMORY_BASIC_INFORMATION* info, char* address, bool* finished) {
    if (VirtualQueryEx(currentProcessHandle, address, info, sizeof(MEMORY_BASIC_INFORMATION)) == 0) {
        DWORD err = GetLastError();
        if (err == 87) {
            *finished = true;
            return false;
        }
        printf("Page query failed with error code %d\n", err);
        return false;
    }
    return true;
}

/*
    Reads memory from address and puts into provided buffer.
*/
bool read_address(void* buf, size_t* sizeBuf, void* address, size_t size) {
    if (ReadProcessMemory(currentProcessHandle, address, buf, size, sizeBuf)) {
        return true;
    }
    printf("Memory read failed with error code %d\n", GetLastError());
    return false;
}

/*
    Read a page and set the address for patterns that match.
    Returns number of unfound patterns left or 0xFF when read_memory fails.
*/
unsigned char search_page(MEMORY_BASIC_INFORMATION info, char* addr) {
    char* buf = malloc(info.RegionSize);
    size_t bufSize;
    if (!read_address((void*)buf, &bufSize, addr, info.RegionSize)) {
        return 0xFF;
    }

    unsigned char unfound_patterns = 0;
    for (int i=0; i<patternCount; i++) {
        SEARCH_PATTERN pttrn = searchPatterns[i];
        if (pttrn.addr != NULL) continue;
        unfound_patterns++;
        
        size_t bufI = 0;
        size_t pttrnI = 0;
        while (bufI < bufSize) {
            if (buf[bufI] == pttrn.pattern[pttrnI]) {
                pttrnI++;
                if (pttrnI == pttrn.size) {
                    searchPatterns[i].addr = (void*)(addr+bufI+1+pttrn.offset);
                    unfound_patterns--;
                    printf("Found pattern %d at 0x%p\n", pttrn.identifier, searchPatterns[i].addr);
                    break;
                }
            } else {
                pttrnI = 0;
            }
            bufI++;
        }
    }

    free(buf);
    return unfound_patterns;
}

/*
    Query through all pages and execute callback for readable pages.
    Returns false when a call to _query_page fails.
*/
bool init_patterns() {
    if (&currentProcessInfo == NULL) {
        printf("Attempted to traverse pages before setting currentProcessInfo\n");
        return false;
    }

    for (int i=0; i<patternCount; i++) {
        SEARCH_PATTERN pttrn = searchPatterns[i];
        if (pttrn.pattern == NULL) {
            searchPatterns[i].addr = (char*)(pttrn.base == 1 ? avs2CoreInfo.lpBaseOfDll : currentProcessInfo.lpBaseOfDll) + pttrn.offset;
        }
    }

    char* addr = currentProcessInfo.lpBaseOfDll;
    MEMORY_BASIC_INFORMATION info;
    bool finished = false;
    
    while (true) {
        if (!query_page(&info, addr, &finished)) return false;
        if (finished) break;
        addr = info.BaseAddress;
        if (!(info.Protect & PAGE_NOACCESS || info.Protect & PAGE_GUARD || info.Protect == 0)) {
            unsigned char result = search_page(info, addr);
            if (result == 0xFF) return false;
            if (result == 0) return true;
        }
        addr += info.RegionSize;
        if ((size_t)addr >= (size_t)currentProcessInfo.lpBaseOfDll + currentProcessInfo.SizeOfImage) {
            break;
        }
    }
    return true;
}

/*
    Looks for and attempts to open a handle to the SDVX process.
    Then find all the memory addresses for the data we want.
*/
bool memory_reader_init() {
    printf("Searching for process\n");
    if (!set_current_process("sv6c.exe")) return false;
    printf("Opening process\n");
    if (!open_process()) return false;
    printf("Getting module information\n");
    if (!init_module_info()) return false;
    printf("Searching pages for wanted memory data\n");
    if (!init_patterns()) return false;
    printf("Memory reader finished initiating\n");
    return true;
}

/*
    Cleanup open process and set data of MemoryData to zero.
*/
bool memory_reader_cleanup() {
    printf("Cleaning up\n");
    memset(&MemoryData, 0, sizeof(MEMORY_DATA));
    if (!close_process()) return false;
    printf("Finished cleaning");
    return true;
}

/*
    Update values of MemoryData
*/
bool memory_reader_update() {
    // TODO: use char instead of string to identify pattern type
    for (int i=0; i<patternCount; i++) {
        SEARCH_PATTERN pttrn = searchPatterns[i];

        size_t sizeBuf;
        if (pttrn.identifier == 0x0) {
            if (!read_address(&MemoryData.GameState, &sizeBuf, pttrn.addr, sizeof(unsigned char))) return false;
        } else if (pttrn.identifier == 0x1 && MemoryData.GameState == STATE_MUSIC_SELECT) {
            // Find location of ui objects from a base pointer
            // First reads return true upon error or null ptr because it's most likely due to
            // transition between screens
            size_t ptr1;
            if (!read_address(&ptr1, &sizeBuf, pttrn.addr, sizeof(size_t))) return true;
            if (ptr1 == 0) return true;
            size_t ptr2;
            if (!read_address(&ptr2, &sizeBuf, (void*)(ptr1+0x174), sizeof(size_t))) return true;
            if (ptr2 == 0) return true;
            size_t ptr3;
            if (!read_address(&ptr3, &sizeBuf, (void*)(ptr2+0x158), sizeof(size_t))) return true;
            if (ptr3 == 0) return true;

            size_t ui_obj_ptrs[26];
            if (!read_address(&ui_obj_ptrs, &sizeBuf, (void*)(ptr3+0x118), sizeof(size_t)*26)) return true;
            for (int i=0; i<26; i++) {
                if (ui_obj_ptrs[i] == 0) return true;
                if (!read_address(MemoryData.UiObjects+i, &sizeBuf, (void*)ui_obj_ptrs[i], sizeof(UI_OBJECT))) return false;

                char* uiText = MemoryData.UiObjects[i].text;
                if (MemoryData.UiObjects[i].text[0] == '[') {
                    int textI = 0;
                    while (textI < 512) {
                        textI++;
                        if (uiText[textI] == ']' && uiText[textI+1] != '[') {
                            break;
                        }
                    }
                    if (textI == 512) {
                        printf("Failed to parse text of ui object %s", MemoryData.UiObjects[i].label);
                        memset(uiText, 0, 512);
                        continue;
                    }
                    char temp[512];
                    memcpy(temp, uiText+textI+1, 512-textI-1);
                    memset(uiText, 0, 512);
                    strcpy(uiText, temp);
                }

                char outBuf[512];
                memset(outBuf, 0, 512);
                if (!decode_text(MemoryData.UiObjects[i].text, outBuf, 512)) return false;
                strcpy(MemoryData.UiObjects[i].text, outBuf);
            }
        }
    }

    return true;
}

DWORD memory_reader_process_id() {
    return currentProcessId;
}
