/*
1. 屏蔽操作（不着急）
2. 速度条、进度条（暂缓）
3. 僵尸Rank（不重要）
*/

#include "iztools.h"
using namespace std;
using namespace chrono;

#include <mingw.std.thread.h> // <thread>
void ClearRndJmp();
void InjectRndRec();
void InjectRecEnd();
void Inject1400Sun();
void ChangeSpeed();
extern HWND hSpeedText;

extern PVOID pCode, pCode2, pData;
extern PVOID pCodeRestart, pCodeCard, pCode1400Sun;
extern PVOID pDataCard, pDataMjClock;
static bool b1400Sun;

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
    DWORD version;
    fin.read((char*)&version, sizeof(version)); // 读取版本
    if(version < REPVERSION) {
        MessageBox(hWnd, "录像文件版本过低，无法识别", "读取失败", MB_ICONERROR);
        return false;
    }
    if(version > REPVERSION) {
        MessageBox(hWnd, "录像文件版本过高，无法识别", "读取失败", MB_ICONERROR);
        return false;
    }
    fin.read((char*)&b1400Sun, sizeof(b1400Sun)); // 是否1400开
    int len; char* data;
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
    // 注入：提示文字显示
    Asm.push(ESI)
        .mov(ESI, (DWORD*)0x6a9ec0)
        .mov(ESI, PESI + 0x768)
        .mov(ESI, PESI + 0x5568)
        .cmp(ESI, 0ul)
        .if_jmp(ABOVE, INJECTOR()
            .cmp((BYTE*)0x70004c, (BYTE)TEXTTYPE::PLAY)
            .if_jmp(EQUAL, INJECTOR()
                .mov((BYTE*)0x70004c, (BYTE)TEXTTYPE::NONE)
                .show_text("Start playing...", 400)
            )
            .cmp((BYTE*)0x70004c, (BYTE)TEXTTYPE::STOP)
            .if_jmp(EQUAL, INJECTOR()
                .mov((BYTE*)0x70004c, (BYTE)TEXTTYPE::NONE)
                .show_text("Stopped.", 400)
            )
            .cmp((BYTE*)0x70004c, (BYTE)TEXTTYPE::END)
            .if_jmp(EQUAL, INJECTOR()
                .mov((BYTE*)0x70004c, (BYTE)TEXTTYPE::NONE)
                .show_text("The video is over!", 400)
            )
        )
        .pop(ESI)
    ;
    // 注入：用卡操作、暂停操作（仅开启暂停）
    Asm.add_byte(0x64).add_word(0x0d89).add_dword(0x0) // original code
        .cmp((BYTE*)0x70001c, 2)
        .if_jmp(NEQUAL, INJECTOR().ret())
        .push(EAX).push(EBX).push(ECX).push(EDX).push(EDI).push(ESI)
        .mov(EAX, (DWORD*)0x700030) // p_data_card
        .cmp(EAX, (DWORD*)0x70002c) // p_data_card_top
        .if_jmp(BELOW, INJECTOR() // 用卡操作
            .mov(ECX, PEAX + 0x08)
            .mov(ESI, (DWORD*)0x6a9ec0)
            .mov(ESI, PESI + 0x768)
            .mov(EBX, PESI + 0x5568)
            .add_word(0xcb39) // cmp ebx, ecx
            .if_jmp(NBELOW, INJECTOR() // 重现用卡操作 (x, y, time, idx, type)
                .add_word(0x70ff).add_byte(4) // push [eax+04]
                .add_word(0x30ff) // push [eax]
                .push(ESI)
                .mov(ECX, PEAX + 0x0c)
                .mov(ESI, PESI + 0x138)
                .mov(PESI + 0x24, ECX)
                .mov(ECX, PEAX + 0x10)
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
            .mov(EDI, PESI + 0x768)
            .mov(EDI, PEDI + 0x5568)
            .mov(EDX, PEAX + 0x04)
            .sub(EDX, 1)
            .add_word(0xfa39) // cmp edx, edi
            .if_jmp(EQUAL, INJECTOR()
                .mov(EDX, PEAX)
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
    Asm.add_word(0xa164).add_dword(0x0) // original code
        .cmp((BYTE*)0x70001c, 2)
        .if_jmp(EQUAL, INJECTOR() // 停止重现
                           .mov((BYTE*)0x70004c, (BYTE)TEXTTYPE::STOP)
                           .mov((BYTE*)0x70001c, 0))
        .cmp((BYTE*)0x70001e, 2)
        .if_jmp(EQUAL, INJECTOR() // 开始重现
                           .mov((BYTE*)0x70004c, (BYTE)TEXTTYPE::PLAY)
                           .mov((BYTE*)0x70001e, 0)
                           .mov((BYTE*)0x70001c, 2));

    pCodeRestart = AllocMemory(Asm.len() + 1);
    Asm.write(pCodeRestart);
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
        Inject1400Sun();
        InjectRepStart();
        std::thread(ChangeSpeed).detach();

        CreateWindow("static", str_replay, WS_VISIBLE | WS_CHILD, 10, 10, 280, 90, hDlg, NULL, hInst, NULL);
        hSpeedText = CreateWindow("static", "", WS_VISIBLE | WS_CHILD, 10, 95, 280, 25, hDlg, NULL, hInst, NULL);
        EnumChildWindows(hDlg, SetChildWndFont, (LPARAM)hFont);
        break;
    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            {
                int a = read_memory<DWORD>(0x700020);
                int b = read_memory<DWORD>(0x700014);
                if(a < b) write_memory<BYTE>((BYTE)TEXTTYPE::STOP, 0x70004c);
            }
            Sleep(50);
            InjectRecEnd();
            ClearRndJmp();
            FreeMemory(pCode);
            FreeMemory(pCode2);
            FreeMemory(pCodeRestart);
            FreeMemory(pCodeCard);
            if(b1400Sun) FreeMemory(pCode1400Sun);
            FreeMemory(pData);
            FreeMemory(pDataCard);
            FreeMemory(pDataMjClock);
            write_memory<BYTE>(0, 0x70001c);
            write_memory<BYTE>(0, 0x70001e);
            write_memory<BYTE>(0, 0x6a9eab);
            break;
        }
        break;
    default:
        return false;
    }
    return true;
}