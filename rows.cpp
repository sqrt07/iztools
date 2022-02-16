#include "iztools.h"

bool Prepare(HWND hWnd);
void InjectCode(int resID);
void RunRemoteCode();

HWND hRowInput;

bool check_rows(const char* s) {
    bool bk[5] = {false};
    for(int i = 0; i < 5; i++) {
        if(s[i] < 0 || s[i] >= 5) return false;
        bk[(int)s[i]] = true;
    }
    for(int i = 0; i < 5; i++)
        if(!bk[i]) return false;
    return true;
}
void set_rows(const char* s, HWND hDlg) {
    if(bRunning) {
        MessageBox(hDlg, "请等待测试完毕。", "提示", MB_OK | MB_ICONWARNING);
        return;
    }
    if(!Prepare(hDlg)) return;
    struct PLANT {
        int row, col, type;
    };
    std::vector<PLANT> v;
    gp.init();
    INJECTOR Asm;
    int cnt_max = read_memory<DWORD>(gp.obj + 0xb0);
    int plant = gp.plant;
    for(int i = 0; i < cnt_max; i++) {
        if(read_memory<bool>(plant + 0x141) == false) {
            int row = read_memory<DWORD>(plant + 0x1c);
            int col = read_memory<DWORD>(plant + 0x28);
            int type = read_memory<DWORD>(plant + 0x24);
            v.push_back({s[row], col, type});
            Asm.del_plant(i);
        }
        plant += 0x14c;
    }
    for(auto p : v) {
        Asm.put_plant(p.row, p.col, p.type);
    }
    Asm.write();
    RunRemoteCode();
}

BOOL CALLBACK RowsDlgProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam) {
    switch(Message) {
    case WM_INITDIALOG: {
        hRowInput = CreateWindow("edit", "54321", WS_VISIBLE | WS_CHILD | ES_NUMBER, 5, 5, 82, 32, hDlg, NULL, hInst, NULL);
        HFONT hf = CreateFont(32, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 32, "Consolas");
        CreateWindow("button", "修改", WS_VISIBLE | WS_CHILD, 90, 8, 50, 24, hDlg, (HMENU)ID_MODIFYROWS, hInst, NULL);
        CreateWindow("button", "翻转", WS_VISIBLE | WS_CHILD, 142, 8, 50, 24, hDlg, (HMENU)ID_REVERSEROWS, hInst, NULL);
        CreateWindow("button", "轮换", WS_VISIBLE | WS_CHILD, 194, 8, 50, 24, hDlg, (HMENU)ID_REVOLVEROWS, hInst, NULL);
        EnumChildWindows(hDlg, SetChildWndFont, (LPARAM)hFont);
        SendMessage(hRowInput, (UINT)WM_SETFONT, (WPARAM)hf, 0);
        return true;
    }
    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case IDCANCEL:
            EnableMenuItem(hMenu, IDM_ROWS, MF_BYCOMMAND | MF_ENABLED);
            EndDialog(hDlg, LOWORD(wParam));
            return true;
        case ID_MODIFYROWS: {
            char s[6];
            GetWindowText(hRowInput, s, 6);
            int len = strlen(s);
            for(int i = 0; i < 5; i++) s[i] -= '1';
            if(len != 5 || !check_rows(s)) {
                MessageBox(hWnd, "输入不合法。", "提示", MB_OK | MB_ICONWARNING);
                break;
            }
            set_rows(s, hDlg);
            break;
        }
        case ID_REVERSEROWS:
            set_rows("\x04\x03\x02\x01\x00", hDlg);
            break;
        case ID_REVOLVEROWS:
            set_rows("\x01\x02\x03\x04\x00", hDlg);
            break;
        }
    }
    return false;
}
