#include <iostream>
#include <windows.h>
using namespace std;

HANDLE hGameProcess = NULL;
const int len = 0x770;
BYTE buf[len];

int main() {
    char fn[MAX_PATH];
    cin >> fn;

    DWORD proc_id;
    GetWindowThreadProcessId(FindWindow("MainWindow", NULL), &proc_id);
    hGameProcess = OpenProcess(PROCESS_ALL_ACCESS, false, proc_id);

    ReadProcessMemory(hGameProcess, (void*)0x651090, buf, len, NULL);
    FILE* fp = fopen(fn, "wb");
    fwrite(buf, 1, len, fp);
    fclose(fp);
    // fp = fopen("resource.rc", "a");
    // fputc('\n', fp);

    return 0;
}
