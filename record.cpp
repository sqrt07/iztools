#include "iztools.h"
using namespace std;
using namespace chrono;

#include <mingw.std.thread.h> // <thread>
void ClearRndJmp();
void InjectRndRec();
void ChangeSpeed();
HWND hSpeedText;

extern PVOID pCode, pCode2, pData;
PVOID pCodeRestart, pCodeCard, pCodeMjClock, pCode1400Sun;
PVOID pDataCard, pDataMjClock;
PVOID pData0, pDataCard0, pDataMjClock0; // 过完一关后的缓存区
extern bool b1400Sun;
static int th_cnt = 0, th_fast = 0;
static const int len_data = 2048 * 1024; // 2M

void SaveRec() {
    char path[MAX_PATH] = "";
    GetModuleFileName(NULL, path, sizeof(path));
    strrchr(path, '\\')[1] = '\0';
    SetCurrentDirectory(path);
    auto t = system_clock::to_time_t(system_clock::now());
    ostringstream sout;
    sout << put_time(localtime(&t), "%Y%m%d-%H-%M-%S");
    const char* dir = "IZERECORD";
    if(access(dir, 0) == -1) mkdir(dir);
    string fn = dir + ("\\" + sout.str()) + ".ize";

    std::ofstream fout(fn, ios::binary);
    DWORD version = REPVERSION; // 版本
    fout.write((const char*)&version, sizeof(version));
    fout.write((const char*)&b1400Sun, sizeof(b1400Sun)); // 是否1400开
    int len = read_memory<int>(0x700028); // 缓存的随机数
    char* data = new char[len];
    ReadProcessMemory(hGameProcess, pData0, data, len, NULL);
    fout.write((const char*)&len, sizeof(len));
    fout.write(data, len);
    delete[] data;
    len = read_memory<int>(0x700034); // 缓存的用卡记录
    data = new char[len];
    ReadProcessMemory(hGameProcess, pDataCard0, data, len, NULL);
    fout.write((const char*)&len, sizeof(len));
    fout.write(data, len);
    delete[] data;
    len = read_memory<int>(0x700048); // 缓存的暂停记录
    data = new char[len];
    ReadProcessMemory(hGameProcess, pDataMjClock0, data, len, NULL);
    fout.write((const char*)&len, sizeof(len));
    fout.write(data, len);
    delete[] data;
    fout.close();

    SetCurrentDirectory(path);
}

void InjectCardRec() {
    pDataCard = AllocMemory(64 * 1024);
    pDataCard0 = AllocMemory(64 * 1024);
    write_memory<void*>(pDataCard, 0x70002c);
    write_memory<void*>(pDataCard, 0x700030);

    INJECTOR Asm;
    // 注入：用卡记录
    Asm.add_word(0x858b).add_dword(0x160)
        .cmp((BYTE*)0x70001c, 1)
        .if_jmp(EQUAL, INJECTOR() // 记录用户用卡操作 (x, y, time, idx, type)
                           .push(EAX).push(EBX).push(ESI)
                           .mov(EAX, (DWORD*)0x70002c)
                           .mov(PEAX, EBX)
                           .mov(PEAX + 0x04, ESI)
                           .mov(ESI, (DWORD*)0x6a9ec0)
                           .mov(ESI, PESI + 0x768)
                           .mov(EBX, PESI + 0x5568)
                           .mov(PEAX + 0x08, EBX)
                           .mov(ESI, PESI + 0x138)
                           .mov(EBX, PESI + 0x24)
                           .mov(PEAX + 0x0c, EBX)
                           .mov(EBX, PESI + 0x28)
                           .mov(PEAX + 0x10, EBX)
                           .add(EAX, 0x14)
                           .mov((DWORD*)0x70002c, EAX)
                           .pop(ESI).pop(EBX).pop(EAX));

    pCodeCard = AllocMemory(Asm.len() + 1);
    Asm.write(pCodeCard);
    write_call<1>(pCodeCard, 0x40fdcf);
}
void InjectMjClockRec() {
    pDataMjClock = AllocMemory(64 * 1024);
    pDataMjClock0 = AllocMemory(64 * 1024);
    write_memory<void*>(pDataMjClock, 0x700040);
    write_memory<void*>(pDataMjClock, 0x700044);

    INJECTOR Asm;
    // 注入：舞王时钟记录
    Asm.add_byte(0x64).add_word(0x0d89).add_dword(0x0) // original code
        .cmp((BYTE*)0x70001c, 1)
        .if_jmp(NEQUAL, INJECTOR().ret())
        .push(EAX).push(ECX).push(EDX).push(ESI)
        .mov(ESI, (DWORD*)0x6a9ec0)
        .mov(EDX, PESI + 0x838)
        .inc((DWORD*)0x70003c) // last_mjclock
        .cmp(EDX, (DWORD*)0x70003c)
        .if_jmp(NEQUAL, INJECTOR()
                            .mov(EAX, (DWORD*)0x700040) // p_data_pause_top
                            .mov(PEAX, EDX)
                            .mov(ESI, PESI + 0x768)
                            .mov(ECX, PESI + 0x5568)
                            .cmp(ECX, 2)
                            .if_jmp(NBELOW, INJECTOR()
                                                .mov((DWORD*)0x70003c, EDX)
                                                .mov(PEAX + 4, ECX)
                                                .add(EAX, 8)
                                                .mov((DWORD*)0x700040, EAX)))
        .pop(ESI).pop(EDX).pop(ECX).pop(EAX);
    // 注入：提示文字显示
    Asm.push(ESI)
        .mov(ESI, (DWORD*)0x6a9ec0)
        .mov(ESI, PESI + 0x768)
        .mov(ESI, PESI + 0x5568)
        .cmp(ESI, 0ul)
        .if_jmp(ABOVE, INJECTOR()
            .cmp((BYTE*)0x70004c, (BYTE)TEXTTYPE::START)
            .if_jmp(EQUAL, INJECTOR()
                .mov((BYTE*)0x70004c, (BYTE)TEXTTYPE::NONE)
                .show_text("Start recording...", 400)
            )
            .cmp((BYTE*)0x70004c, (BYTE)TEXTTYPE::SAVE)
            .if_jmp(EQUAL, INJECTOR()
                .mov((BYTE*)0x70004c, (BYTE)TEXTTYPE::NONE)
                .show_text("Video saved! Start recording...", 400)
            )
        )
        .pop(ESI)
    ;

    pCodeMjClock = AllocMemory(Asm.len() + 1);
    Asm.write(pCodeMjClock);
    write_call<2>(pCodeMjClock, 0x41606f);
}
void InjectRecStart() {
    pData0 = AllocMemory(len_data);
    write_memory<void*>(pData, 0x700014);
    write_memory<void*>(pData, 0x700020);

    INJECTOR Asm;
    // 注入：重开时记录随机数
    Asm.add_word(0xa164).add_dword(0x0) // original code
        .cmp((BYTE*)0x70001c, 1)
        .if_jmp(EQUAL, INJECTOR() // 保存全部记录到缓存
                           .mov((BYTE*)0x70004c, (BYTE)TEXTTYPE::SAVE)
                           .push(ECX).push(ESI).push(EDI)
                           .mov(ECX, (DWORD*)0x700014)
                           .sub(ECX, (int)pData).push(ECX)
                           .mov(ESI, (int)pData)
                           .mov(EDI, (int)pData0)
                           .repe().movsb()
                           .pop(ECX).mov((DWORD*)0x700028, ECX) // 保存随机数
                           .mov(ECX, (DWORD*)0x70002c)
                           .sub(ECX, (int)pDataCard).push(ECX)
                           .mov(ESI, (int)pDataCard)
                           .mov(EDI, (int)pDataCard0)
                           .repe().movsb()
                           .pop(ECX).mov((DWORD*)0x700034, ECX) // 保存用卡操作
                           .mov(ECX, (DWORD*)0x700040)
                           .sub(ECX, (int)pDataMjClock).push(ECX)
                           .mov(ESI, (int)pDataMjClock)
                           .mov(EDI, (int)pDataMjClock0)
                           .repe().movsb()
                           .pop(ECX).mov((DWORD*)0x700048, ECX) // 保存暂停操作
                           .pop(EDI).pop(ESI).pop(ECX)
                           .mov((DWORD*)0x700014, (int)pData)
                           .mov((DWORD*)0x70002c, (int)pDataCard)
                           .mov((DWORD*)0x700040, (int)pDataMjClock)
                           .mov((DWORD*)0x70003c, -1))
        .cmp((BYTE*)0x70001e, 1)
        .if_jmp(EQUAL, INJECTOR() // 开始录制
                           .mov((BYTE*)0x70004c, (BYTE)TEXTTYPE::START)
                           .mov((BYTE*)0x70001e, 0)
                           .mov((BYTE*)0x70001c, 1));

    pCodeRestart = AllocMemory(Asm.len() + 1);
    Asm.write(pCodeRestart);
    write_call<1>(pCodeRestart, 0x407b57);
}
void Inject1400Sun() {
    INJECTOR Asm;
    // 注入：使用1400开局
    Asm.add_word(0x82c7).add_dword(0x608).add_dword(0x0) // original code
        .push(EAX).push(EBX).push(ECX).push(EDX).push(ESI).push(EDI)
        .cmp((BYTE*)0x70001c, 0)
        .if_jmp(ABOVE, INJECTOR()
            .mov(EDI, (DWORD*)0x6a9ec0)
            .mov(EDI, PEDI + 0x768)
            .mov(EAX, PEDI + 0x5568)
            .mov(EDI, PEDI + 0x160)
            .cmp(EAX, 0ul)
            .if_jmp(EQUAL, INJECTOR()
                .mov(EAX, (DWORD*)0x6a9ec0)
                .mov(EAX, PEAX + 0x768)
                .mov(EDI, PEAX + 0x160)
                .push(EDI)
                .add(EAX, 0xac)
                .mov(EBX, 0x41e590).add_word(0xd3ff) // call PlantList_Clear
                .pop(EDI)
                ._new_plant(1, 1, m_p["h"])
                ._new_plant(1, 2, m_p["h"])
                ._new_plant(1, 5, m_p["h"])
                ._new_plant(2, 1, m_p["h"])
                ._new_plant(3, 2, m_p["h"])
                ._new_plant(4, 2, m_p["h"])
                ._new_plant(5, 1, m_p["h"])
                ._new_plant(5, 2, m_p["h"])
                ._new_plant(1, 4, m_p["j"])
                ._new_plant(3, 3, m_p["l"])
                ._new_plant(4, 3, m_p["l"])
                ._new_plant(4, 4, m_p["l"])
                ._new_plant(3, 4, m_p["2"])
                ._new_plant(2, 4, m_p["y"])
                ._new_plant(4, 5, m_p["y"])
                ._new_plant(5, 4, m_p["y"])
                ._new_plant(5, 3, m_p["3"])
                ._new_plant(2, 5, m_p["b"])
                ._new_plant(3, 5, m_p["b"])
                ._new_plant(5, 5, m_p["b"])
                ._new_plant(4, 1, m_p["s"])
                ._new_plant(3, 1, m_p["c"])
                ._new_plant(1, 3, m_p["_"])
                ._new_plant(2, 2, m_p["_"])
                ._new_plant(2, 3, m_p["_"])))
        .pop(EDI).pop(ESI).pop(EDX).pop(ECX).pop(EBX).pop(EAX);

    pCode1400Sun = AllocMemory(Asm.len() + 1);
    Asm.write(pCode1400Sun);
    write_call<5>(pCode1400Sun, 0x42b27c);
}
void InjectRecEnd() {
    write_code({0x8b, 0x85, 0x60, 0x01, 0x00, 0x00}, 0x40fdcf);
    write_code({0x64, 0xa1, 0x00, 0x00, 0x00, 0x00}, 0x407b57);
    write_code({0x64, 0x89, 0x0d, 0x00, 0x00, 0x00, 0x00}, 0x41606f);
    write_code({0xc3, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc}, 0x40dfb0);
    write_code({0xc7, 0x82, 0x08, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0x42b27c);
}
BOOL CALLBACK RecDlgProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch(Message) {
    case WM_INITDIALOG:
        write_memory<BYTE>(0, 0x70001c);  // flag_remember
        write_memory<BYTE>(1, 0x70001e);  // flag_recorder
        write_memory<int>(0, 0x700028);   // len_rec
        write_memory<int>(0, 0x700034);   // len_rec_card
        write_memory<int>(-1, 0x70003c);  // last_mjclock
        pData = AllocMemory(len_data);
        write_memory<void*>(pData, 0x700020); // p_data
        InjectRndRec();
        InjectCardRec();
        InjectMjClockRec();
        if(b1400Sun) Inject1400Sun();
        InjectRecStart();
        std::thread([] {
            const int cnt = ++th_cnt;
            while(th_cnt == cnt) {
                if(read_memory<int>(0x700028) > 0) {
                    SaveRec();
                    write_memory<int>(0, 0x700028);
                }
                Sleep(1000);
            }
        }).detach();
        std::thread(ChangeSpeed).detach();

        CreateWindow("static", str_record, WS_VISIBLE | WS_CHILD, 10, 10, 280, 70, hDlg, NULL, hInst, NULL);
        hSpeedText = CreateWindow("static", "", WS_VISIBLE | WS_CHILD, 10, 75, 280, 25, hDlg, NULL, hInst, NULL);
        EnumChildWindows(hDlg, SetChildWndFont, (LPARAM)hFont);
        SetFocus(hDlg);
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
            FreeMemory(pCodeMjClock);
            if(b1400Sun) FreeMemory(pCode1400Sun);
            FreeMemory(pData);
            FreeMemory(pData0);
            FreeMemory(pDataCard);
            FreeMemory(pDataCard0);
            FreeMemory(pDataMjClock);
            FreeMemory(pDataMjClock0);
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

void ChangeSpeed() {
    const int cnt = ++th_fast;
    while(th_fast == cnt) {
        if(GetKeyState(VK_UP) & 0x8000) {
            while(GetKeyState(VK_UP) & 0x8000) Sleep(10);
            if(th_fast != cnt) break;
            int now = read_memory<DWORD>(0x4526d3);
            if(!read_memory<bool>(0x6a9eab)) now = 1;
            ++now;
            write_memory<DWORD>(now, 0x4526d3);
            write_memory<BYTE>(1, 0x6a9eab);
            char s[64];
            sprintf(s, "当前速度：%d倍", now);
            SetWindowText(hSpeedText, s);
        }
        else if(GetKeyState(VK_DOWN) & 0x8000) {
            while(GetKeyState(VK_DOWN) & 0x8000) Sleep(50);
            int now = read_memory<DWORD>(0x4526d3);
            if(!read_memory<bool>(0x6a9eab)) now = 1;
            if(now > 1) --now;
            write_memory<DWORD>(now, 0x4526d3);
            write_memory<BYTE>(1, 0x6a9eab);
            char s[64];
            sprintf(s, "当前速度：%d倍", now);
            SetWindowText(hSpeedText, s);
        }
        Sleep(10);
    }
}