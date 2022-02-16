#include "iztools.h"

HINSTANCE hInst;
HWND hWnd;
HMENU hMenu;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    srand(time(0));

    hInst = hInstance;
    WNDCLASS wc = {};
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.style = CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = "iztools";
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(ICON_BRAIN));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszMenuName = MAKEINTRESOURCE(IDM_MENU);

    RegisterClass(&wc);
    RECT rect = {0, 0, 284, 465};
    AdjustWindowRect(&rect, WS_CAPTION | WS_SYSMENU, true);
    hWnd = CreateWindow(wc.lpszClassName, "IZ²âÊÔ¹¤¾ß " VERSION, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, hInstance, NULL);
    if(hWnd == NULL) {
        MessageBox(NULL, "Fatal Error!", "Fatal Error!", MB_OK | MB_ICONEXCLAMATION);
        return 1;
    }
    hMenu = GetMenu(hWnd);
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        if(msg.message == WM_KEYDOWN && msg.wParam == VK_TAB) {
            HWND hFocus = GetFocus();
            if(hFocus != NULL && hFocus != hWnd && (GetWindowLong(hFocus, GWL_STYLE) & WS_TABSTOP)) {
                HWND hNext = hFocus;
                do {
                    hNext = GetNextWindow(hNext, GW_HWNDNEXT);
                    if(hNext == NULL)
                        hNext = GetNextWindow(hFocus, GW_HWNDFIRST);
                } while((GetWindowLong(hNext, GWL_STYLE) & WS_TABSTOP) == 0);
                SetFocus(hNext);
                continue;
            }
        }
        if(msg.message == WM_CHAR && msg.wParam == VK_TAB) continue;
        DispatchMessage(&msg);
    }
    return msg.wParam;
}
