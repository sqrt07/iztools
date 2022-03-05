#include "iztools.h"
#include "data.h"

HWND hDropdown, hDataInput;

BOOL CALLBACK DataDlgProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    try {
        switch(Message) {
        case WM_INITDIALOG: {
            ::hDlg = hDlg;
            hDropdown = CreateWindow("combobox", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | CBS_HASSTRINGS | WS_OVERLAPPED, 5, 5, 100, 200, hDlg, NULL, hInst, NULL);
            for(const MEMDATA& memd : memdatas)
                SendMessage(hDropdown, CB_ADDSTRING, 0, (LPARAM)memd.info);
            hDataInput = CreateWindow("edit", "", WS_VISIBLE | WS_CHILD | ES_NUMBER, 105, 7, 72, 25, hDlg, NULL, hInst, NULL);
            CreateWindow("button", "ÐÞ¸Ä", WS_VISIBLE | WS_CHILD, 182, 7, 50, 25, hDlg, (HMENU)ID_CHANGEDATA, hInst, NULL);
            CreateWindow("button", "»¹Ô­", WS_VISIBLE | WS_CHILD, 235, 7, 50, 25, hDlg, (HMENU)ID_DEFAULTDATA, hInst, NULL);
            EnumChildWindows(hDlg, SetChildWndFont, (LPARAM)hFont);
            return true;
        }
        case WM_COMMAND:
            if(HIWORD(wParam) == CBN_SELCHANGE) {
                int idx = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                int data = memdatas[idx].read();
                char s[20];
                sprintf(s, "%d", data);
                SetWindowText(hDataInput, s);
            }
            switch(LOWORD(wParam)) {
            case IDCANCEL:
                EnableMenuItem(hMenu, IDM_DATA, MF_BYCOMMAND | MF_ENABLED);
                EndDialog(hDlg, LOWORD(wParam));
                return true;
            case ID_CHANGEDATA: {
                char s[20], s2[100];
                GetWindowText(hDataInput, s, sizeof(s));
                GetWindowText(hDropdown, s2, sizeof(s2));
                for(const MEMDATA& memd : memdatas) {
                    if(strcmp(memd.info, s2) == 0) {
                        memd.write(atoi(s));
                        break;
                    }
                }
                break;
            }
            case ID_DEFAULTDATA: {
                char s[20], s2[100];
                GetWindowText(hDropdown, s2, sizeof(s2));
                for(const MEMDATA& memd : memdatas) {
                    if(strcmp(memd.info, s2) == 0) {
                        memd.write(memd.defval);
                        sprintf(s, "%d", memd.read());
                        SetWindowText(hDataInput, s);
                        break;
                    }
                }
                break;
            }
            }
        }
    } catch(int) {}
    return false;
}
