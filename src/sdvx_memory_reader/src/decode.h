#ifndef DECODE_H
#define DECODE_H

#include "stdio.h"
#include "stdbool.h"

#define reverse_order(c) ((c >> 8) + ((c << 8) & 0xff00))

bool decode_text(char* text, char* outBuf, size_t bufSize);

#endif