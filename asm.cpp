#include "iztools.h"

extern HANDLE hGameProcess;

GAMEPTR gp;

void INJECTOR::put_plant(int row, int col, int type) {
    push(-1);
    push(type);
    push(col);
    push(gp.obj);
    mov(EAX, row);
    call(0x40d120);
    push(EAX);
    mov(EAX, gp.prog);
    call(0x42a530);
}
void INJECTOR::del_plant(int i) {
    int p = gp.plant + i * 0x14c;
    push(p);
    call(0x4679b0);
}
void INJECTOR::del_zombie(int i) {
    int p = gp.zombie + i * 0x15c;
    write_memory<DWORD>(1, p + 0x28);
}
void INJECTOR::del_bullet(int i) {
    int p = gp.bullet + i * 0x94;
    mov(EAX, p);
    call(0x46eb20);
}
void INJECTOR::clear_plants() {
    int cnt_max = read_memory<DWORD>(gp.obj + 0xb0);
    for(int i = 0; i < cnt_max; i++)
        if(read_memory<bool>(gp.plant + i * 0x14c + 0x141) == false)
            del_plant(i);
}
void INJECTOR::clear_zombies() {
    int cnt_max = read_memory<DWORD>(gp.obj + 0x94);
    for(int i = 0; i < cnt_max; i++)
        del_zombie(i);
}
void INJECTOR::clear_bullets() {
    int cnt_max = read_memory<DWORD>(gp.obj + 0xcc);
    for(int i = 0; i < cnt_max; i++)
        if(read_memory<bool>(gp.bullet + i * 0x94 + 0x50) == false)
            del_bullet(i);
}
void INJECTOR::write(void* addr) {
    ret();
    WriteProcessMemory(hGameProcess, addr, code, pos, NULL);
}