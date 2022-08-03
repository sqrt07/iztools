#pragma once
#include <windows.h>
#include <initializer_list>
#include <cstring>

#ifdef SCRIPT_DLL
    static HANDLE hGameProcess;
#else
    extern HANDLE hGameProcess;
#endif

template <typename T, typename... Args>
T read_memory(T* ptr, Args... args) {
    std::initializer_list<int> adrs = {args...};
    BYTE* p = (BYTE*)ptr;
    for(auto adr : adrs) {
        ReadProcessMemory(hGameProcess, p, &p, sizeof(p), NULL);
        p += adr;
    }
    T ans;
    ReadProcessMemory(hGameProcess, p, &ans, sizeof(ans), NULL);
    return ans;
}
template <typename T, typename... Args>
T read_memory(int ptr, Args... args) {
    return read_memory((T*)ptr, args...);
}

template <typename T, typename... Args>
void write_memory(T value, T* ptr, Args... args) {
    std::initializer_list<int> adrs = {args...};
    BYTE* p = (BYTE*)ptr;
    for(auto adr : adrs) {
        ReadProcessMemory(hGameProcess, p, &p, sizeof(p), NULL);
        p += adr;
    }
    WriteProcessMemory(hGameProcess, p, &value, sizeof(value), NULL);
}
template <typename T, typename... Args>
void write_memory(T value, int ptr, Args... args) {
    write_memory(value, (T*)ptr, args...);
}

template <int NOP = 0>
void write_call(void* func, int ptr) {
    BYTE code[NOP + 5];
    memset(code, 0x90, sizeof(code));
    code[0] = 0xe8;
    *(int*)&code[1] = (int)func - ptr - 5;
    WriteProcessMemory(hGameProcess, (void*)ptr, code, sizeof(code), NULL);
}
template <int NOP = 0>
void write_jmp(void* func, int ptr) {
    BYTE code[NOP + 5];
    memset(code, 0x90, sizeof(code));
    code[0] = 0xe9;
    *(int*)&code[1] = (int)func - ptr - 5;
    WriteProcessMemory(hGameProcess, (void*)ptr, code, sizeof(code), NULL);
}

inline void write_code(const std::initializer_list<BYTE>& code, int ptr) {
    WriteProcessMemory(hGameProcess, (void*)ptr, code.begin(), code.size(), NULL);
}
