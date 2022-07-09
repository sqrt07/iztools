#include "iztools.h"

PVOID pCodeCollect;

void InjectCollector() {
    INJECTOR Asm;
    // 注入：用卡操作、初始栈位设置
    Asm.add_byte(0x8b).add_word(0x248c).add_dword(0x114) // original code
        .cmp((BYTE*)0x70001f, 1)
        .if_jmp(NEQUAL, INJECTOR().ret())
        .push(EAX).push(ECX)
        .mov(ECX, (DWORD*)0x6a9ec0)
        .add_word(0x898b).add_dword(0x768) // mov ecx, [ecx+768]
        .add_word(0x818b).add_dword(0xe8) // mov eax, [ecx+e8]
        .add_word(0x898b).add_dword(0xe4) // mov ecx, [ecx+e4]
        .cmp(EAX, 0ul) // flag
        .if_jmp(ABOVE, INJECTOR()
                           .push(EAX).push(ECX)
                           .push(1)
                           .mov(EAX, 0x432c00).add_word(0xd0ff) // call Item_HandCollect
                           .pop(ECX).pop(EAX)
                           .add_byte(0x48) // dec eax
                           .add(ECX, 0xd8)
                           .jmp(-0x23)) // jmp flag
        .pop(ECX).pop(EAX);

    write_code({0x8b, 0x8c, 0x24, 0x14, 0x01, 0x00, 0x00}, 0x416064);
    FreeMemory(pCodeCollect);
    pCodeCollect = AllocMemory(Asm.len() + 1);
    Asm.write(pCodeCollect);
    write_call<2>(pCodeCollect, 0x416064);
}