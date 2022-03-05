#include "iztools.h"

#include <mingw.std.thread.h> // <thread>
#include <sys/timeb.h>

extern int start_time;
HWND hResText, hTimeText;
bool UpdateResult(HWND hDlg, int sec) {
    bool update = sec % 10 == 0;
    sec /= 10;
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
        EndTest();
        MessageBeep(0);
        tm = read_memory<DWORD>(0x6ffff8);
        sprintf(s, "平均速度：%.2lf", (tm - start_time) / (double)(sec * 100));
        SetWindowText(hTimeText, s);
    } else if(update) {
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
BOOL CALLBACK DlgProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch(Message) {
    case WM_INITDIALOG:
        hResText = CreateWindow("edit", "0 / 0", WS_VISIBLE | WS_CHILD | ES_READONLY, 10, 5, 200, 20, hDlg, NULL, hInst, NULL);
        CreateWindow("button", "复制", WS_VISIBLE | WS_CHILD, 10, 30, 50, 25, hDlg, (HMENU)ID_CPYRES, hInst, NULL);
        hTimeText = CreateWindow("edit", "", WS_VISIBLE | WS_CHILD | ES_READONLY, 10, 60, 200, 20, hDlg, NULL, hInst, NULL);
        hFont = CreateFont(20, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 32, "Microsoft YaHei");
        EnumChildWindows(hDlg, SetChildWndFont, (LPARAM)hFont);
        timeb tb;
        ftime(&tb);
        static long long t = 0;
        {
            long long tnow = tb.time * 1000ll + tb.millitm;
            if(tnow - t < 120) Sleep(120 - (tnow - t));
            t = tnow;
        }
        std::thread([hDlg] {
            int sec = 0;
            while(UpdateResult(hDlg, sec++))
                Sleep(100);
        }).detach();
        break;
    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case IDCANCEL:
            EndTest();
            EndDialog(hDlg, LOWORD(wParam));
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
        }
        break;
    default:
        return false;
    }
    return true;
}
