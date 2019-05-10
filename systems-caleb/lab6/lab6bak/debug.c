#include <stdio.h>
#include <stdarg.h>
#include "debug.h"

void debug(const char* format, ...) {
    if (DEBUG) {
        va_list arguements;
        va_start(arguements, format);
        vfprintf(stderr, format, arguements);
        va_end(arguements); 
    }

}
