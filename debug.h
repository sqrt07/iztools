#pragma once
#include <windows.h>
#include <cstdio>

inline void debug(int x) {
    char s[100];
    sprintf(s, "%d", x);
    MessageBox(NULL, s, "", MB_OK);
}
inline void debug(const char* x) {
    MessageBox(NULL, x, "", MB_OK);
}