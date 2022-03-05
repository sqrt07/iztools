#include <windows.h>
#include "memory.h"

bool Prepare(HWND hWnd, bool gameui = true);
static HWND hDlg;

struct MEMDATA {
    const char* info;
    DWORD addr;
    int len;
    int defval;
    int read() const {
        if(!Prepare(hDlg, false)) throw 0;
        int data = 0;
        ReadProcessMemory(hGameProcess, (void*)addr, &data, len, NULL);
        return data;
    }
    void write(int data) const {
        if(!Prepare(hDlg, false)) throw 0;
        WriteProcessMemory(hGameProcess, (void*)addr, &data, len, NULL);
    }
};

const MEMDATA memdatas[] = {
    {"[hp]С��", 0x5235ac, 4, 70},
    {"[hp]��ͨ", 0x5227bb, 4, 270},
    {"[hp]·��", 0x522892, 4, 370},
    {"[hp]�Ÿ�", 0x522cbf, 4, 500},
    {"[hp]��Ͱ", 0x52292b, 4, 1100},
    {"[hp]�ļ�", 0x522a1b, 4, 450},
    {"[hp]��", 0x522bef, 4, 100},
    {"[hp]����", 0x52299c, 4, 500},
    {"[hp]���", 0x522bb0, 4, 1400},
    {"[hp]����", 0x523530, 4, 500},
};