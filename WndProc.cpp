#include "iztools.h"
#include <psapi.h>

HFONT hFont;
HWND hPlantBox;
HWND hPlantType[5];
HWND hPlantNo[5];
const char* plantTypes[] = {"冰豆", "分裂", "土豆", "大嘴", "大嘴"};
const char* plantNo[] = {"1", "2", "3", "4", "5"};
HWND hZombieBox;
HWND hZombieType[5], hZombieTime[5], hZombieCol[5];
const char* zombieTypes[] = {"小鬼", "小鬼", "扶梯", "撑杆", ""};
const char* zombieTimes[] = {"0", "0", "200", "500", ""};
const char* zombieCols[] = {"6", "6", "6", "6", ""};
HWND hClear, hDefault, hHelp;
HWND hToolBox;
HWND hTextKeyPlant, hBtnSwitch;
HWND h5TestInput, hMjlockInput, hTextMjlock;
HWND hBtnClear, hBtnDefault, hBtnCopy, hBtnPaste;
extern const char* str_5test;

HWND hCountInput, hKeyPlantInput;
const char* countMax = "1000";
const char* keyPlant = "1";

bool bSpeed = true, bHalfSpeed, bNoInject, bDelay460, bRunning;
bool b5Test, bDLL, bDelayInf, bShowMe, bFreePlanting;
bool bVBECard, bVBNoRepeater, bVBEShowPlants;
bool bCollector, b1400Sun, b1400Warning;

extern PVOID pCodeCollect;

BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK RowsDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DataDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK RecDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK RepDlgProc(HWND, UINT, WPARAM, LPARAM);
bool ReadEditText();
bool Prepare(HWND hWnd, bool gameui = true);
void GetString(char*);
void SetString(const char*);
bool ReadTestStr(const char*);
void StartTest();
bool Start5Test();

BOOL CALLBACK SetChildWndFont(HWND hWnd, LPARAM lParam) {
    SendMessage(hWnd, WM_SETFONT, (WPARAM)lParam, 1);
    EnumChildWindows(hWnd, SetChildWndFont, lParam);
    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam) {
    switch(Message) {
    case WM_CREATE: {
        ::hWnd = hWnd;
        CheckMenuItem(GetMenu(hWnd), IDM_SPEED, MF_BYCOMMAND | MF_CHECKED);
        InitTextMap();
        hFont = CreateFont(20, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 32, "Microsoft YaHei");
        hPlantBox = CreateWindow("button", "植物", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 10, 10, 264, 80, hWnd, NULL, hInst, NULL);
        CreateWindow("static", "类型：", WS_VISIBLE | WS_CHILD, 9, 20, 50, 18, hPlantBox, NULL, hInst, NULL);
        CreateWindow("static", "编号：", WS_VISIBLE | WS_CHILD, 9, 50, 50, 18, hPlantBox, NULL, hInst, NULL);
        for(int i = 0; i < 5; i++)
            hPlantType[i] = CreateWindow("edit", plantTypes[i], WS_VISIBLE | WS_CHILD | ES_CENTER | WS_TABSTOP, 60 + 40 * i, 20, 36, 22, hPlantBox, NULL, hInst, NULL);
        for(int i = 0; i < 5; i++)
            hPlantNo[i] = CreateWindow("edit", plantNo[i], WS_VISIBLE | WS_CHILD | ES_CENTER | WS_TABSTOP | ES_NUMBER, 60 + 40 * i, 50, 36, 22, hPlantBox, NULL, hInst, NULL);
        hZombieBox = CreateWindow("button", "僵尸", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 10, 100, 264, 110, hWnd, NULL, hInst, NULL);
        CreateWindow("static", "类型：", WS_VISIBLE | WS_CHILD, 9, 20, 50, 18, hZombieBox, NULL, hInst, NULL);
        CreateWindow("static", "时刻：", WS_VISIBLE | WS_CHILD, 9, 50, 50, 18, hZombieBox, NULL, hInst, NULL);
        CreateWindow("static", "列：", WS_VISIBLE | WS_CHILD, 9, 80, 50, 18, hZombieBox, NULL, hInst, NULL);
        for(int i = 0; i < 5; i++)
            hZombieType[i] = CreateWindow("edit", zombieTypes[i], WS_VISIBLE | WS_CHILD | ES_CENTER | WS_TABSTOP, 60 + 40 * i, 20, 36, 22, hZombieBox, NULL, hInst, NULL);
        for(int i = 0; i < 5; i++)
            hZombieTime[i] = CreateWindow("edit", zombieTimes[i], WS_VISIBLE | WS_CHILD | ES_CENTER | WS_TABSTOP | ES_NUMBER, 60 + 40 * i, 50, 36, 22, hZombieBox, NULL, hInst, NULL);
        for(int i = 0; i < 5; i++)
            hZombieCol[i] = CreateWindow("edit", zombieCols[i], WS_VISIBLE | WS_CHILD | ES_CENTER | WS_TABSTOP | ES_NUMBER, 60 + 40 * i, 80, 36, 22, hZombieBox, NULL, hInst, NULL);
        CreateWindow("static", "测试总数：", WS_VISIBLE | WS_CHILD, 12, 220, 80, 24, hWnd, NULL, hInst, NULL);
        hCountInput = CreateWindow("edit", "1000", WS_VISIBLE | WS_CHILD | ES_CENTER | ES_NUMBER, 90, 220, 55, 22, hWnd, NULL, hInst, NULL);
        hTextKeyPlant = CreateWindow("static", "目标列：", WS_VISIBLE | WS_CHILD, 170, 220, 70, 24, hWnd, NULL, hInst, NULL);
        hKeyPlantInput = CreateWindow("edit", "1", WS_VISIBLE | WS_CHILD | ES_CENTER | ES_NUMBER, 230, 220, 35, 22, hWnd, NULL, hInst, NULL);
        hTextMjlock = CreateWindow("static", "舞王锁定：", WS_VISIBLE | WS_CHILD, 156, 220, 70, 24, hWnd, NULL, hInst, NULL);
        hMjlockInput = CreateWindow("edit", "", WS_VISIBLE | WS_CHILD | ES_CENTER | ES_NUMBER, 230, 220, 35, 22, hWnd, NULL, hInst, NULL);
        CreateWindow("button", "开始", WS_VISIBLE | WS_CHILD, 9, 250, 50, 24, hWnd, (HMENU)ID_TOGGLE, hInst, NULL);
        hBtnClear = CreateWindow("button", "清空", WS_VISIBLE | WS_CHILD, 9 + 60, 250, 50, 24, hWnd, (HMENU)ID_CLEAR, hInst, NULL);
        hBtnDefault = CreateWindow("button", "默认", WS_VISIBLE | WS_CHILD, 9 + 60 * 2, 250, 50, 24, hWnd, (HMENU)ID_DEFAULT, hInst, NULL);
        hBtnCopy = CreateWindow("button", "复制", WS_VISIBLE | WS_CHILD, 9 + 60, 250, 50, 24, hWnd, (HMENU)ID_COPY, hInst, NULL);
        hBtnPaste = CreateWindow("button", "粘贴", WS_VISIBLE | WS_CHILD, 9 + 60 * 2, 250, 50, 24, hWnd, (HMENU)ID_PASTE, hInst, NULL);
        hBtnSwitch = CreateWindow("button", "全场测试", WS_VISIBLE | WS_CHILD, 9 + 60 * 3, 250, 84, 24, hWnd, (HMENU)ID_5TEST, hInst, NULL);
        CreateWindow("button", "辅助", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 10, 280, 264, 170, hWnd, NULL, hInst, NULL);
        EnumChildWindows(hWnd, SetChildWndFont, (LPARAM)hFont);
        h5TestInput = CreateWindow("edit", str_5test, WS_VISIBLE | WS_CHILD | ES_MULTILINE | ES_AUTOHSCROLL, 18, 15, 250, 195, hWnd, NULL, hInst, NULL);
        HFONT hf = CreateFont(20, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 32, "Consolas");
        SendMessage(h5TestInput, WM_SETFONT, (WPARAM)hf, 1);
        for(HWND h : {h5TestInput, hMjlockInput, hTextMjlock, hBtnCopy, hBtnPaste})
            ShowWindow(h, SW_HIDE);
        break;
    }
    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case ID_TOGGLE: {
            char input_str[1024] = "";
            if(b5Test) GetString(input_str);
            if(!(b5Test ? ReadTestStr(input_str) : ReadEditText())) {
                MessageBox(hWnd, "输入不合法。", "提示", MB_OK | MB_ICONWARNING);
                break;
            }
            if(!Prepare(hWnd)) break;
            if(b5Test) {
                if(!Start5Test()) break;
            }else StartTest();
            DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG), hWnd, (DLGPROC)DlgProc);
            break;
        }
        case ID_CLEAR:
            for(int i = 0; i < 5; i++) {
                SetWindowText(hPlantType[i], "");
                SetWindowText(hZombieType[i], "");
                SetWindowText(hZombieTime[i], "");
                SetWindowText(hZombieCol[i], "");
            }
            break;
        case ID_DEFAULT:
            for(int i = 0; i < 5; i++) {
                SetWindowText(hPlantType[i], plantTypes[i]);
                SetWindowText(hZombieType[i], zombieTypes[i]);
                SetWindowText(hZombieTime[i], zombieTimes[i]);
                SetWindowText(hZombieCol[i], zombieCols[i]);
            }
            break;
        case IDM_SPEED: {
            int flag = bSpeed ? MF_UNCHECKED : MF_CHECKED;
            CheckMenuItem(hMenu, IDM_SPEED, MF_BYCOMMAND | flag);
            if(!bSpeed) CheckMenuItem(hMenu, IDM_HALFSPEED, MF_BYCOMMAND | MF_UNCHECKED);
            bSpeed = !bSpeed;
            bHalfSpeed = false;
            break;
        }
        case IDM_HALFSPEED: {
            int flag = bHalfSpeed ? MF_UNCHECKED : MF_CHECKED;
            CheckMenuItem(hMenu, IDM_HALFSPEED, MF_BYCOMMAND | flag);
            if(!bHalfSpeed) CheckMenuItem(hMenu, IDM_SPEED, MF_BYCOMMAND | MF_UNCHECKED);
            bHalfSpeed = !bHalfSpeed;
            bSpeed = false;
            break;
        }
        case IDM_DLL: {
            int flag = bDLL ? MF_UNCHECKED : MF_CHECKED;
            CheckMenuItem(hMenu, IDM_DLL, MF_BYCOMMAND | flag);
            bDLL = !bDLL;
            break;
        }
        case IDM_DELAYINF: {
            int flag = bDelayInf ? MF_UNCHECKED : MF_CHECKED;
            CheckMenuItem(hMenu, IDM_DELAYINF, MF_BYCOMMAND | flag);
            bDelayInf = !bDelayInf;
            break;
        }
        case IDM_DELAY460: {
            int flag = bDelay460 ? MF_UNCHECKED : MF_CHECKED;
            CheckMenuItem(hMenu, IDM_DELAY460, MF_BYCOMMAND | flag);
            bDelay460 = !bDelay460;
            break;
        }
        case IDM_NOINJECT: {
            int flag = bNoInject ? MF_UNCHECKED : MF_CHECKED;
            CheckMenuItem(hMenu, IDM_NOINJECT, MF_BYCOMMAND | flag);
            bNoInject = !bNoInject;
            break;
        }
        case IDM_CARD: {
            if(!Prepare(hWnd)) break;
            int flag = bVBECard ? MF_UNCHECKED : MF_CHECKED;
            CheckMenuItem(hMenu, IDM_CARD, MF_BYCOMMAND | flag);
            bVBECard = !bVBECard;
            write_memory<DWORD>(bVBECard ? 0x90909090 : 0x01544583, 0x430dce);
            break;
        }
        case IDM_SHOWPLANTS: {
            if(!Prepare(hWnd)) break;
            int flag = bVBEShowPlants ? MF_UNCHECKED : MF_CHECKED;
            CheckMenuItem(hMenu, IDM_SHOWPLANTS, MF_BYCOMMAND | flag);
            bVBEShowPlants = !bVBEShowPlants;
            write_memory<BYTE>(bVBEShowPlants ? 19 : 2, 0x4294de);
            break;
        }
        case IDM_VBNOREPEATER: {
             if(!Prepare(hWnd)) break;
            int flag = bVBNoRepeater ? MF_UNCHECKED : MF_CHECKED;
            CheckMenuItem(hMenu, IDM_VBNOREPEATER, MF_BYCOMMAND | flag);
            bVBNoRepeater = !bVBNoRepeater;
            write_memory<BYTE>(bVBNoRepeater ? 37 : 52, 0x4293d8);
            break;
        }
        case IDM_SHOWME: {
            HWND hWnd = FindWindow("Qt5150QWindowIcon", "ShowMe 1.1");
            if(!hWnd) break;
            DWORD proc_id;
            GetWindowThreadProcessId(hWnd, &proc_id);
            HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, false, proc_id);
            if(!hProc) break;
            BYTE old_code[] = {0xff, 0x15, 0x28, 0x20, 0x49, 0x00};
            BYTE code[] = {0x83, 0xc4, 0x14, 0x90, 0x90, 0x90};
            /* 获取起始地址 */
            HMODULE hModule = NULL;
            DWORD dwRet = 0;
            EnumProcessModules(hProc, &hModule, sizeof(hModule), &dwRet);
            WriteProcessMemory(hProc, (void*)((DWORD)hModule + 0x2ab4), bShowMe ? old_code : code, sizeof(code), nullptr);
            int flag = bShowMe ? MF_UNCHECKED : MF_CHECKED;
            CheckMenuItem(hMenu, IDM_SHOWME, MF_BYCOMMAND | flag);
            bShowMe = !bShowMe;
            break;
        }
        case IDM_FREEPLANTING: {
            if(!Prepare(hWnd)) break;
            int flag = bFreePlanting ? MF_UNCHECKED : MF_CHECKED;
            CheckMenuItem(hMenu, IDM_FREEPLANTING, MF_BYCOMMAND | flag);
            bFreePlanting = !bFreePlanting;
            write_memory<BYTE>(bFreePlanting, 0x6a9ec0, 0x814);
            break;
        }
        case IDM_HELP:
            MessageBox(NULL, str_help, "单行帮助", MB_OK | MB_ICONINFORMATION);
            break;
        case IDM_HELP5:
            MessageBox(NULL, str_help5, "全场帮助", MB_OK | MB_ICONINFORMATION);
            break;
        case IDM_CHAR:
            MessageBox(NULL, str_char, "字符对照表", MB_OK | MB_ICONINFORMATION);
            break;
        case IDM_ABOUT:
            MessageBox(NULL, str_about, "关于...", MB_OK | MB_ICONINFORMATION);
            break;
        case IDM_ROWS:
            EnableMenuItem(hMenu, IDM_ROWS, MF_BYCOMMAND | MF_DISABLED);
            CreateDialog(hInst, MAKEINTRESOURCE(IDD_ROWS), hWnd, (DLGPROC)RowsDlgProc);
            break;
        case IDM_DATA:
            EnableMenuItem(hMenu, IDM_DATA, MF_BYCOMMAND | MF_DISABLED);
            CreateDialog(hInst, MAKEINTRESOURCE(IDD_DATA), hWnd, (DLGPROC)DataDlgProc);
            break;
        case IDM_COLLECTOR: {
            if(!Prepare(hWnd)) break;
            int flag = bCollector ? MF_UNCHECKED : MF_CHECKED;
            CheckMenuItem(hMenu, IDM_COLLECTOR, MF_BYCOMMAND | flag);
            bCollector = !bCollector;
            write_memory<BYTE>(bCollector, 0x70001f);
            break;
        }
        case IDM_RECORD:
            if(!Prepare(hWnd)) break;
            write_memory<BYTE>(1, 0x70001f);
            DialogBox(hInst, MAKEINTRESOURCE(IDD_RECORD), hWnd, (DLGPROC)RecDlgProc);
            write_memory<BYTE>(bCollector, 0x70001f);
            break;
        case IDM_REPLAY:
            if(!Prepare(hWnd)) break;
            write_memory<BYTE>(1, 0x70001f);
            DialogBox(hInst, MAKEINTRESOURCE(IDD_REPLAY), hWnd, (DLGPROC)RepDlgProc);
            write_memory<BYTE>(bCollector, 0x70001f);
            break;
        case IDM_1400SUN: {
            int flag = b1400Sun ? MF_UNCHECKED : MF_CHECKED;
            if(!b1400Sun && !b1400Warning) {
                b1400Warning = true;
                int ret = MessageBox(hWnd, str_1400warning, "提示", MB_YESNO | MB_ICONWARNING);
                if(ret != IDYES) break;
            }
            CheckMenuItem(hMenu, IDM_1400SUN, MF_BYCOMMAND | flag);
            b1400Sun = !b1400Sun;
            break;
        }
        case ID_5TEST:
            for(HWND h : {hPlantBox, hZombieBox, hTextKeyPlant, hKeyPlantInput, hBtnClear, hBtnDefault})
                ShowWindow(h, b5Test ? SW_SHOW : SW_HIDE);
            for(HWND h : {h5TestInput, hMjlockInput, hTextMjlock, hBtnCopy, hBtnPaste})
                ShowWindow(h, b5Test ? SW_HIDE : SW_SHOW);
            SetWindowText(hBtnSwitch, b5Test ? "全场测试" : "单行测试");
            b5Test = !b5Test;
            break;
        case ID_COPY: {
            char s[1024];
            GetString(s);
            int len = strlen(s);
            HGLOBAL hMem = GlobalAlloc(GHND, len + 1);
            char* lpMem = (char*)GlobalLock(hMem);
            memcpy(lpMem, s, len);
            GlobalUnlock(hMem);
            OpenClipboard(hWnd);
            EmptyClipboard();
            SetClipboardData(CF_TEXT, hMem);
            CloseClipboard();
            break;
        }
        case ID_PASTE: {
            OpenClipboard(hWnd);
            HANDLE hMem = GetClipboardData(CF_TEXT);
            int len = GlobalSize(hMem);
            const char* lpMem = (char*)GlobalLock(hMem);
            char* s = new char[len];
            memcpy(s, lpMem, len);
            GlobalUnlock(hMem);
            CloseClipboard();
            SetString(s);
            delete[] s;
            break;
        }
        }
        break;
    case WM_CLOSE:
    case WM_DESTROY:
        write_code({0x8b, 0x8c, 0x24, 0x14, 0x01, 0x00, 0x00}, 0x416064);
        FreeMemory(pCodeCollect);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, Message, wParam, lParam);
    }
    return 0;
}
