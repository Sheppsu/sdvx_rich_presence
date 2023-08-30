#include "decode.h"
#include "jis.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <wchar.h>

unsigned char wchar_to_utf8(char* buf, wchar_t wchar) {
    if (wchar <= 0x7F) {
        buf[0] = (unsigned char)wchar;
        return 1;
    } else if (wchar <= 0x7FF) {
        buf[0] = (0xC0 | ((wchar >> 6) & 0x1F));
        buf[1] = (0x80 | (wchar & 0x3F));
        return 2;
    } else {
        buf[0] = (0xE0 | ((wchar >> 12) & 0x0F));
        buf[1] = (0x80 | ((wchar >> 6) & 0x3F));
        buf[2] = (0x80 | (wchar & 0x3F));
        return 3;
    }
}

bool decode_text(char* text, char* outBuf, size_t bufSize) {
    int textI = 0;
    int bufI = 0;
    size_t textLen = strlen(text);
    while (textI < textLen) {
        wchar_t wchar = textI == textLen-1 ? 0 : reverse_order(*((wchar_t*)(text+textI)));
        bool isJis = false;
        for (int i=0; i<DECODING.numGroups; i++) {
            DECODE_GROUP group = *DECODING.groups[i];
            if (wchar >= group.minVal && wchar <= group.maxVal) {
                for (int k=0; k<group.numPairs; k++) {
                    DECODE_PAIR pair = group.pairs[k];
                    if (pair.jis == wchar) {
                        bufI += wchar_to_utf8(outBuf+bufI, pair.unicode);
                        textI += 2;
                        isJis = true;
                        break;
                    }
                }
                break;
            }
        }

        if (!isJis) {
            outBuf[bufI] = text[textI];
            textI += 1;
            bufI += 1;
        }

        if (bufI >= bufSize) {
            printf("bufI exceeded provided buffer size");
            return false;
        }
    }

    return true;
}
