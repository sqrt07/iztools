/*
1. ���β��������ż���
2. �ٶ��������������ݻ���
3. ��ʬRank������Ҫ��
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
    ofn.lpstrFilter = "IZE¼���ļ�(.ize)\0*.ize\0";
    ofn.nFilterIndex = 0;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if(!GetOpenFileName(&ofn)) return false;

    ifstream fin(fn, ios::binary);
    int preview, len; char* data;
    fin.read((char*)&preview, sizeof(preview)); // ��ʼջλ����
    write_memory<int>(preview, 0x700038);
    fin.read((char*)&len, sizeof(len)); // ���������
    pData = AllocMemory(len + 5);
    write_memory<void*>(pData, 0x700020);
    write_memory<int>((int)pData + len, 0x700014);
    if(len) {
        data = new char[len];
        fin.read(data, len);
        WriteProcessMemory(hGameProcess, pData, data, len, NULL);
        delete data;
    }
    fin.read((char*)&len, sizeof(len)); // �ÿ���������
    pDataCard = AllocMemory(len + 5);
    write_memory<void*>(pDataCard, 0x700030);
    write_memory<int>((int)pDataCard + len, 0x70002c);
    if(len) {
        data = new char[len];
        fin.read(data, len);
        WriteProcessMemory(hGameProcess, pDataCard, data, len, NULL);
        delete data;
    }
    fin.read((char*)&len, sizeof(len)); // ��ͣ��������
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
    // ע�룺��ʾ������ʾ
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
    // ע�룺�ÿ���������ʼջλ���á���ͣ��������������ͣ��
    Asm.add_byte(0x64).add_word(0x0d89).add_dword(0x0) // original code
        .cmp((BYTE*)0x70001c, 2)
        .if_jmp(NEQUAL, INJECTOR().ret())
        .push(EAX).push(EBX).push(ECX).push(EDX).push(EDI).push(ESI)
        .mov(ESI, (DWORD*)0x6a9ec0)
        .mov(ESI, PESI + 0x768)
        .mov(EAX, PESI + 0x5568)
        .cmp(EAX, 2) // �ڶ�֡�ų������Ԥ����ʬ
        .if_jmp(EQUAL, INJECTOR() // ��ʼջλ����
            .mov(EAX, (DWORD*)0x700038) // preview_top (8 or 9)
            .mov(PESI + 0x94, EAX) // count_max
            .add_byte(0x48) // dec eax
            .mov(PESI + 0x9c, EAX) // next
            .mov(ESI, PESI + 0x90)
            .add_byte(0x40) // inc eax
            .mov(PESI + 0x158, EAX) // zb[0].next
            .mov(EAX, 7)
            .mov(PESI + 8 * 0x15c + 0x158, EAX) // zb[8].next
        )
        .mov(EAX, (DWORD*)0x700030) // p_data_card
        .cmp(EAX, (DWORD*)0x70002c) // p_data_card_top
        .if_jmp(BELOW, INJECTOR() // �ÿ�����
            .mov(ECX, PEAX + 0x08)
            .mov(ESI, (DWORD*)0x6a9ec0)
            .mov(ESI, PESI + 0x768)
            .mov(EBX, PESI + 0x5568)
            .add_word(0xcb39) // cmp ebx, ecx
            .if_jmp(NBELOW, INJECTOR() // �����ÿ����� (x, y, time, idx, type)
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
    // ע�룺�ؿ�ʱ���������
    Asm.add_word(0xa164).add_dword(0x0) // original code
        .cmp((BYTE*)0x70001c, 2)
        .if_jmp(EQUAL, INJECTOR() // ֹͣ����
                           .mov((BYTE*)0x70004c, (BYTE)TEXTTYPE::STOP)
                           .mov((BYTE*)0x70001c, 0))
        .cmp((BYTE*)0x70001e, 2)
        .if_jmp(EQUAL, INJECTOR() // ��ʼ����
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
        InjectRepStart();
        break;
    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            write_memory<BYTE>((BYTE)TEXTTYPE::STOP, 0x70004c);
            Sleep(50);
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