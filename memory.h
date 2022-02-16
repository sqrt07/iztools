#pragma once
#include <windows.h>
#include <initializer_list>

#ifdef SCRIPT_DLL
    static HANDLE hGameProcess;
#else
    extern HANDLE hGameProcess;
#endif

template <typename T, typename... Args>
T read_memory(Args... args) {
    T ans = T();
    int ptr = 0;
    std::initializer_list<int> adrs = {args...};
    for(auto it = adrs.begin(); it != adrs.end(); it++) {
        if(it == adrs.end() - 1)
            ReadProcessMemory(hGameProcess, (void*)(ptr + *it), &ans, sizeof(ans), NULL);
        else
            ReadProcessMemory(hGameProcess, (void*)(ptr + *it), &ptr, sizeof(ptr), NULL);
    }
    return ans;
}

template <typename T, typename... Args>
void write_memory(T value, Args... args) {
    int ptr = 0;
    std::initializer_list<int> adrs = {args...};
    for(auto it = adrs.begin(); it != adrs.end(); it++) {
        if(it == adrs.end() - 1)
            WriteProcessMemory(hGameProcess, (void*)(ptr + *it), &value, sizeof(value), NULL);
        else
            ReadProcessMemory(hGameProcess, (void*)(ptr + *it), &ptr, sizeof(ptr), NULL);
    }
}
