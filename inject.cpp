#include "iztools.h"

int LoadCode(int resID, LPBYTE& ptr) {
    HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(resID), RT_RCDATA);
    ptr = (LPBYTE)LockResource(LoadResource(NULL, hRes));
    return SizeofResource(NULL, hRes);
}
void InjectCode(int resID) {
    LPBYTE code;
    int len = LoadCode(resID, code);
    WriteProcessMemory(hGameProcess, (LPVOID)0x651090, code, len, NULL);
}
void RunRemoteCode() {
    HANDLE th = CreateRemoteThread(hGameProcess, NULL, 0, LPTHREAD_START_ROUTINE(0x651090), NULL, 0, NULL);
    WaitForSingleObject(th, INFINITE);
    CloseHandle(th);
}

void* AllocMemory(int size) {
    void* p = VirtualAllocEx(hGameProcess, NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    return p;
}
void FreeMemory(void*& p) {
    if(p) VirtualFreeEx(hGameProcess, p, 0, MEM_RELEASE);
    p = nullptr;
}