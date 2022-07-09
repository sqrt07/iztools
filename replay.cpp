/*
1. 屏蔽操作（不着急）
2. 提示文字、结束提示（游戏内白字）
3. 速度条、进度条（暂缓）
4. 僵尸Rank（不重要）
*/

#include "iztools.h"
using namespace std;
using namespace chrono;

#include <mingw.std.thread.h> // <thread>
void ClearRndJmp();
void InjectRndRec();
void InjectRecEnd();

extern PVOID pCode, pCode2, pData;
extern PVOID pCodeRestart, pCodeCard, pCodePreview;
extern PVOID pDataCard, pDataMjClock;

bool LoadRec(HWND hWnd) {
    OPENFILENAME ofn = {};
    char fn[MAX_PATH] = "";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = fn;
    ofn.nMaxFile = sizeof(fn);
    ofn.lpstrFilter = "IZE录像文件(.ize)\0*.ize\0";
    ofn.nFilterIndex = 0;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if(!GetOpenFileName(&ofn)) return false;

    ifstream fin(fn, ios::binary);
    int preview, len; char* data;
    fin.read((char*)&preview, sizeof(preview)); // 初始栈位载入
    write_memory<int>(preview, 0x700038);
    fin.read((char*)&len, sizeof(len)); // 随机数载入
    pData = AllocMemory(len + 5);
    write_memory<void*>(pData, 0x700020);
    write_memory<int>((int)pData + len, 0x700014);
    if(len) {
        data = new char[len];
        fin.read(data, len);
        WriteProcessMemory(hGameProcess, pData, data, len, NULL);
        delete data;
    }
    fin.read((char*)&len, sizeof(len)); // 用卡操作载入
    pDataCard = AllocMemory(len + 5);
    write_memory<void*>(pDataCard, 0x700030);
    write_memory<int>((int)pDataCard + len, 0x70002c);
    if(len) {
        data = new char[len];
        fin.read(data, len);
        WriteProcessMemory(hGameProcess, pDataCard, data, len, NULL);
        delete data;
    }
    fin.read((char*)&len, sizeof(len)); // 暂停操作载入
    pDataMjClock = AllocMemory(len + 5);
    write_memory<void*>(pDataMjClock, 0x700044);
    write_memory<int>((int)pDataMjClock + len, 0x700040);
    if(len) {
        data = new char[len];
        fin.read(data, len);
        WriteProcessMemory(hGameProcess, pDataMjClock, data, len, NULL);
        delete data;
    }

    fin.close();
    return true;
}

void InjectCardRep() {
    INJECTOR Asm;
    // 注入：用卡操作、初始栈位设置、暂停操作（仅开启暂停）
    Asm.add_byte(0x64).add_word(0x0d89).add_dword(0x0) // original code
        .cmp((BYTE*)0x70001c, 2)
        .if_jmp(NEQUAL, INJECTOR().ret())
        .push(EAX).push(EBX).push(ECX).push(EDX).push(EDI).push(ESI)
        .mov(ESI, (DWORD*)0x6a9ec0)
        .add_word(0xb68b).add_dword(0x768) // mov esi, [esi+768]
        .add_word(0x868b).add_dword(0x5568) // mov eax, [esi+5568]
        .cmp(EAX, 2) // 第二帧才彻底清除预览僵尸
        .if_jmp(EQUAL, INJECTOR() // 初始栈位设置
            .mov(EAX, (DWORD*)0x700038) // preview_top (8 or 9)
            .mov(PESI + 0x94, EAX) // count_max
            .add_byte(0x48) // dec eax
            .mov(PESI + 0x9c, EAX) // next
            .add_word(0xb68b).add_dword(0x90) // mov esi, [esi+90]
            .add_byte(0x40) // inc eax
            .mov(PESI + 0x158, EAX) // zb[0].next
            .mov(EAX, 7)
            .mov(PESI + 8 * 0x15c + 0x158, EAX) // zb[8].next
        )
        .mov(EAX, (DWORD*)0x700030) // p_data_card
        .cmp(EAX, (DWORD*)0x70002c) // p_data_card_top
        .if_jmp(BELOW, INJECTOR() // 用卡操作
            .add_word(0x488b).add_byte(0x08) // mov ecx, [eax+08]
            .mov(ESI, (DWORD*)0x6a9ec0)
            .add_word(0xb68b).add_dword(0x768) // mov esi, [esi+768]
            .add_word(0x9e8b).add_dword(0x5568) // mov ebx, [esi+5568]
            .add_word(0xcb39) // cmp ebx, ecx
            .if_jmp(NBELOW, INJECTOR() // 重现用卡操作 (x, y, time, idx, type)
                .add_word(0x70ff).add_byte(4) // push [eax+04]
                .add_word(0x30ff) // push [eax]
                .push(ESI)
                .add_word(0x488b).add_byte(0x0c) // mov ecx, [eax+0c]
                .add_word(0xb68b).add_dword(0x138) // mov esi, [esi+138]
                .mov(PESI + 0x24, ECX)
                .add_word(0x488b).add_byte(0x10) // mov ecx, [eax+10]
                .mov(PESI + 0x28, ECX)
                .mov(ECX, 1)
                .add(EAX, 0x14)
                .mov((DWORD*)0x700030, EAX)
                .mov(EAX, 0x40fd30).add_word(0xd0ff) // call Board_UseCard
            )
        )
        .mov(EAX, (DWORD*)0x700044) // p_data_pause
        .cmp(EAX, (DWORD*)0x700040) // p_data_pause_top
        .if_jmp(BELOW, INJECTOR()
            .mov(ESI, (DWORD*)0x6a9ec0)
            .add_word(0xbe8b).add_dword(0x768) // mov edi, [esi+768]
            .add_word(0xbf8b).add_dword(0x5568) // mov edi, [edi+5568]
            .add_word(0x508b).add_byte(0x4) // mov edx, [eax+04]
            .sub(EDX, 1)
            .add_word(0xfa39) // cmp edx, edi
            .if_jmp(EQUAL, INJECTOR()
                .add_word(0x108b) // mov edx, [eax]
                .sub(EDX, 1)
                .mov(PESI + 0x838, EDX)
                .add(EAX, 8)
                .mov((DWORD*)0x700044, EAX)
            )
        )
        .pop(ESI).pop(EDI).pop(EDX).pop(ECX).pop(EBX).pop(EAX);

    pCodeCard = AllocMemory(Asm.len() + 1);
    Asm.write(pCodeCard);
    write_call<2>(pCodeCard, 0x41606f);
}
void InjectRepStart() {
    INJECTOR Asm;
    // 注入：重开时重现随机数
    // Asm.mov(EAX, 5) // original code
    Asm.add_word(0xa164).add_dword(0x0) // original code
        // .add_byte(0x8b).add_word(0x6c4f) // mov ecx, [edi+6c]
        // .cmp(ECX, 0ul)
        // .if_jmp(NEQUAL, INJECTOR().ret())
        .cmp((BYTE*)0x70001c, 2)
        .if_jmp(EQUAL, INJECTOR() // 停止重现
                           .mov((BYTE*)0x70001c, 0))
        .cmp((BYTE*)0x70001e, 2)
        .if_jmp(EQUAL, INJECTOR() // 开始重现
                           .mov((BYTE*)0x70001e, 0)
                           .mov((BYTE*)0x70001c, 2));

    pCodeRestart = AllocMemory(Asm.len() + 1);
    Asm.write(pCodeRestart);
    // write_call(pCodeRestart, 0x42af46);
    write_call<1>(pCodeRestart, 0x407b57);
}
BOOL CALLBACK RepDlgProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch(Message) {
    case WM_INITDIALOG:
        if(!LoadRec(hDlg)) {
            EndDialog(hDlg, IDCANCEL);
            break;
        };
        write_memory<BYTE>(0, 0x70001c);  // flag_remember
        write_memory<BYTE>(2, 0x70001e);  // flag_recorder

        InjectRndRec();
        InjectCardRep();
        InjectRepStart();
        break;
    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            InjectRecEnd();
            ClearRndJmp();
            FreeMemory(pCode);
            FreeMemory(pCode2);
            FreeMemory(pCodeRestart);
            FreeMemory(pCodeCard);
            FreeMemory(pData);
            FreeMemory(pDataCard);
            FreeMemory(pDataMjClock);
            write_memory<BYTE>(0, 0x70001c);
            write_memory<BYTE>(0, 0x70001e);
            break;
        }
        break;
    default:
        return false;
    }
    return true;
}