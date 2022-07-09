#include "iztools.h"
using namespace std;
using namespace chrono;

#include <mingw.std.thread.h> // <thread>
void ClearRndJmp();
void InjectRndRec();

extern PVOID pCode, pCode2, pData;
PVOID pCodeRestart, pCodeCard, pCodePreview, pCodeMjClock;
PVOID pDataCard, pDataMjClock;
PVOID pData0, pDataCard0, pDataMjClock0; // 过完一关后的缓存区
static int th_cnt = 0;
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
    int preview = read_memory<int>(0x700038); // 记录的初始栈位
    fout.write((const char*)&preview, sizeof(preview));
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
                           .add_word(0xb68b).add_dword(0x768) // mov esi, [esi+768]
                           .add_word(0x9e8b).add_dword(0x5568) // mov ebx, [esi+5568]
                           .mov(PEAX + 0x08, EBX)
                           .add_word(0xb68b).add_dword(0x138) // mov esi, [esi+138]
                           .add_word(0x5e8b).add_byte(0x24) // mov ebx, [esi+24]
                           .mov(PEAX + 0x0c, EBX)
                           .add_word(0x5e8b).add_byte(0x28) // mov ebx, [esi+28]
                           .mov(PEAX + 0x10, EBX)
                           .add(EAX, 0x14)
                           .mov((DWORD*)0x70002c, EAX)
                           .pop(ESI).pop(EBX).pop(EAX));

    pCodeCard = AllocMemory(Asm.len() + 1);
    Asm.write(pCodeCard);
    write_call<1>(pCodeCard, 0x40fdcf);
}
void InjectPreviewRec() {
    INJECTOR Asm;
    // 注入：初始栈位记录
    Asm.cmp((BYTE*)0x70001c, 1)
        .if_jmp(EQUAL, INJECTOR()
                           .push(ESI)
                           .mov(ESI, (DWORD*)0x6a9ec0)
                           .add_word(0xb68b).add_dword(0x768) // mov esi, [esi+768]
                           .add_word(0xb68b).add_dword(0x94) // mov esi, [esi+94]
                           .mov((DWORD*)0x700038, ESI) // preview_top (8 or 9)
                           .pop(ESI));

    pCodePreview = AllocMemory(Asm.len() + 1);
    Asm.write(pCodePreview);
    write_memory<BYTE>(0xc3, 0x40dfb0 + 5);
    write_call(pCodePreview, 0x40dfb0);
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
        .add_word(0x968b).add_dword(0x838) // mov edx, [esi+838]
        .inc((DWORD*)0x70003c) // last_mjclock
        .cmp(EDX, (DWORD*)0x70003c)
        .if_jmp(NEQUAL, INJECTOR()
                            .mov(EAX, (DWORD*)0x700040) // p_data_pause_top
                            .mov(PEAX, EDX)
                            .add_word(0xb68b).add_dword(0x768) // mov esi, [esi+768]
                            .add_word(0x8e8b).add_dword(0x5568) // mov ecx, [esi+5568]
                            .cmp(ECX, 2)
                            .if_jmp(NBELOW, INJECTOR()
                                                .mov((DWORD*)0x70003c, EDX)
                                                .mov(PEAX + 4, ECX)
                                                .add(EAX, 8)
                                                .mov((DWORD*)0x700040, EAX)))
        .pop(ESI).pop(EDX).pop(ECX).pop(EAX);

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
    // Asm.mov(EAX, 5) // original code
    Asm.add_word(0xa164).add_dword(0x0) // original code
        // .add_byte(0x8b).add_word(0x6c4f) // mov ecx, [edi+6c]
        // .cmp(ECX, 0ul)
        // .if_jmp(NEQUAL, INJECTOR().ret())
        .cmp((BYTE*)0x70001c, 1)
        .if_jmp(EQUAL, INJECTOR() // 保存全部记录到缓存
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
                           .mov((BYTE*)0x70001e, 0)
                           .mov((BYTE*)0x70001c, 1));

    pCodeRestart = AllocMemory(Asm.len() + 1);
    Asm.write(pCodeRestart);
    // write_call(pCodeRestart, 0x42af46);
    write_call<1>(pCodeRestart, 0x407b57);
}
void InjectRecEnd() {
    write_code({0x8b, 0x85, 0x60, 0x01, 0x00, 0x00}, 0x40fdcf);
    // write_code({0xb8, 0x05, 0x00, 0x00, 0x00}, 0x42af46);
    write_code({0x64, 0xa1, 0x00, 0x00, 0x00, 0x00}, 0x407b57);
    write_code({0x64, 0x89, 0x0d, 0x00, 0x00, 0x00, 0x00}, 0x41606f);
    write_code({0xc3, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc}, 0x40dfb0);
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
        InjectPreviewRec();
        InjectMjClockRec();
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
            FreeMemory(pCodePreview);
            FreeMemory(pCodeMjClock);
            FreeMemory(pData);
            FreeMemory(pData0);
            FreeMemory(pDataCard);
            FreeMemory(pDataCard0);
            FreeMemory(pDataMjClock);
            FreeMemory(pDataMjClock0);
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