#include "iztools.h"

#include <mingw.std.thread.h> // <thread>

extern int start_time;
extern bool b5Test, bDLLSuccess;
extern PVOID pData, pData2;
HWND hResText, hTimeText;
HWND hReplay[3], hBtnReplay, hBtnSave;
int test_cnt = 0;
void EndResult(HWND hDlg) {
    if(!hReplay[0]) return;
    for(HWND& h : hReplay)
        ShowWindow(h, SW_HIDE);
    ShowWindow(hBtnReplay, SW_SHOW);
    ShowWindow(hBtnSave, SW_SHOW);
}
bool UpdateResult(HWND hDlg, int sec, int tcnt) {
    if(test_cnt - tcnt > 1) return false;
    int res, cnt, flg, tm;
    char s[1024];
    res = read_memory<int>(0x70000c);
    cnt = read_memory<int>(0x700004);
    flg = read_memory<bool>(0x700000);
    sprintf(s, "%d / %d", res, cnt);
    if(cnt) sprintf(s, "%s [%.2lf%%]", s, 100.0f * res / cnt);
    if(!flg) {
        SetWindowText(hResText, s);
        sprintf(s, "Done. (%ds)", sec);
        SetWindowText(hDlg, s);
        if(test_cnt % 2) {
            MessageBeep(0);
            ++test_cnt;
            EndTest();
        }
        tm = read_memory<DWORD>(0x6ffff8);
        sprintf(s, "平均速度：%.2lf", (tm - start_time) / (double)(sec * 100));
        SetWindowText(hTimeText, s);
        EndResult(hDlg);
    } else {
        SetWindowText(hResText, s);
        sprintf(s, "Running... (%ds)", sec);
        SetWindowText(hDlg, s);
        if(sec > 0) {
            tm = read_memory<DWORD>(0x6a9ec0, 0x768, 0x5568);
            sprintf(s, "平均速度：%.2lf", (tm - start_time) / (double)(sec * 100));
            SetWindowText(hTimeText, s);
        }
    }
    return flg;
}
void SaveData() {
    std::ofstream fout("data.out");
    DWORD* pTop = read_memory<DWORD*>(0x700014);
    for(DWORD* p = (DWORD*)pData; p < pTop; ++p) {
        DWORD d = read_memory<DWORD>(p);
        if(d < 10000) fout << d << '\n';
        else fout << *(float*)&d << '\n';
    }
    fout.close();
}
void SaveData2() {
    std::ofstream fout("data2.out");
    DWORD* pTop = (DWORD*)((BYTE*)pData2 + read_memory<int>(0x700050));
    for(DWORD* p = (DWORD*)pData2; p < pTop; ++p) {
        DWORD d = read_memory<DWORD>(p);
        fout << std::hex << d << '\n';
    }
    fout.close();
}
BOOL CALLBACK DlgProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch(Message) {
    case WM_INITDIALOG:
        hResText = CreateWindow("edit", "0 / 0", WS_VISIBLE | WS_CHILD | ES_READONLY, 10, 5, 200, 20, hDlg, NULL, hInst, NULL);
        CreateWindow("button", "复制", WS_VISIBLE | WS_CHILD, 10, 30, 50, 25, hDlg, (HMENU)ID_CPYRES, hInst, NULL);
        if(b5Test) {
            CreateWindow("button", "直接成功", WS_VISIBLE | WS_CHILD, 10, 85, 80, 25, hDlg, (HMENU)ID_SETWIN, hInst, NULL);
            CreateWindow("button", "直接失败", WS_VISIBLE | WS_CHILD, 95, 85, 80, 25, hDlg, (HMENU)ID_SETLOSE, hInst, NULL);
            hReplay[0] = CreateWindow("button", "默认", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP, 10, 145, 150, 25, hDlg, (HMENU)ID_NOREPLAY, hInst, NULL);
            hReplay[1] = CreateWindow("button", "失败终止", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON, 10, 172, 150, 25, hDlg, (HMENU)ID_LOSEREPLAY, hInst, NULL);
            hReplay[2] = CreateWindow("button", "成功终止", WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON, 10, 199, 150, 25, hDlg, (HMENU)ID_WINREPLAY, hInst, NULL);
            hBtnReplay = CreateWindow("button", "回放", WS_VISIBLE | WS_CHILD, 10, 145, 50, 25, hDlg, (HMENU)ID_BTNREPLAY, hInst, NULL);
            // hBtnSave = CreateWindow("button", "导出", WS_VISIBLE | WS_CHILD, 65, 145, 50, 25, hDlg, (HMENU)ID_BTNSAVE, hInst, NULL);
            ShowWindow(hBtnReplay, SW_HIDE);
            ShowWindow(hBtnSave, SW_HIDE);
            SendMessage(hReplay[0], BM_SETCHECK, 1, 0);
        }
        hTimeText = CreateWindow("edit", "", WS_VISIBLE | WS_CHILD | ES_READONLY, 10, 60, 200, 20, hDlg, NULL, hInst, NULL);
        if(bDLLSuccess)
            CreateWindow("static", "DLL加载成功", WS_VISIBLE | WS_CHILD, 10, 115, 120, 25, hDlg, 0, hInst, NULL);
        hFont = CreateFont(20, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 32, "Microsoft YaHei");
        EnumChildWindows(hDlg, SetChildWndFont, (LPARAM)hFont);

        ++test_cnt;
        std::thread([hDlg] {
            int sec = 0, cnt = test_cnt;
            while(UpdateResult(hDlg, sec++, cnt))
                Sleep(1000);
        }).detach();
        break;
    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            if(test_cnt % 2){
                ++test_cnt;
                EndTest();
            } else {
                EndTest(true);
            }
            break;
        case ID_CPYRES: {
            char s[32];
            GetWindowText(hResText, s, 32);
            int len = strlen(s);
            HGLOBAL hMem = GlobalAlloc(GHND, len + 1);
            char* lpMem = (char*)GlobalLock(hMem);
            memcpy(lpMem, s, len);
            GlobalUnlock(hMem);
            OpenClipboard(hDlg);
            EmptyClipboard();
            SetClipboardData(CF_TEXT, hMem);
            CloseClipboard();
            break;
        }
        case ID_SETWIN:
            write_memory<BYTE>(2, 0x700002);
            break;
        case ID_SETLOSE:
            write_memory<BYTE>(1, 0x700002);
            break;
        case ID_NOREPLAY:
            write_memory<BYTE>(0, 0x70001d);
            break;
        case ID_LOSEREPLAY:
            write_memory<BYTE>(1, 0x70001d);
            break;
        case ID_WINREPLAY:
            write_memory<BYTE>(2, 0x70001d);
            break;
        case ID_BTNREPLAY:
            // EnableWindow(hBtnReplay, false);
            write_memory<DWORD>(0, 0x700050);
            write_memory<void*>(pData, 0x700020);
            write_memory<BYTE>(1, 0x6a9ec0, 0x814);
            write_memory<BYTE>(3, 0x70001c);   // flag_data
            write_memory<BYTE>(1, 0x700001);   // flag_start
            write_memory<BYTE>(1, 0x700000);   // flag
            write_memory<BYTE>(0, 0x700002);   // flag_state
            write_memory<DWORD>(0, 0x70000C);  // result
            write_memory<DWORD>(0, 0x700004);  // cur_count
            write_memory<DWORD>(1, 0x6ffff4);  // count_max
            write_memory<DWORD>(0x23b562e9, 0x415b29);  // 跳转
            break;
        case ID_BTNSAVE:
            SaveData();
            SaveData2();
            break;
        }
        break;
    default:
        return false;
    }
    return true;
}
