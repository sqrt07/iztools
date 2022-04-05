#include "iztools.h"

void InjectCode(int resID);

HANDLE hGameProcess = NULL;
HWND hGameWindow = NULL;
extern HMODULE hDLL;
extern HWND hCountInput, hKeyPlantInput;
extern HWND hPlantType[5], hZombieType[5], hZombieTime[5], hZombieCol[5];
extern HWND hPlantNo[5];

extern bool bSpeed, bHalfSpeed, bNoInject, b5Test, bDelayInf, bDLL;

DWORD count_max = 1000, key_plant_col = 1;
BYTE plants_type[5], plants_col[5], zombies_type[5], zombies_col[5];
WORD zombies_time[5];
int plants_no[5];
BYTE zombie_cnt;

int start_time;

inline int GetIntFrom(HWND hwnd) {
    int t = 0;
    char s[20];
    GetWindowText(hwnd, s, 20);
    sscanf(s, "%d", &t);
    return t;
}
bool ReadEditText() {
    count_max = GetIntFrom(hCountInput);
    if(count_max <= 0 || count_max % 5 != 0) return false;
    key_plant_col = GetIntFrom(hKeyPlantInput);
    if(key_plant_col > 5) return false;
    for(int i = 0; i < 5; i++) {
        char s[20];
        GetWindowText(hPlantType[i], s, 20);
        std::string str(s);
        if(count(str.begin(), str.end(), ' ') == (int)str.length()) {
            plants_type[i] = 0xff;
            continue;
        }
        int t = FindPlantInMap(str);
        if(t == -1) return false;
        plants_type[i] = t;
    }
    if(plants_type[key_plant_col - 1] == 0xff) return false;
    bool b = false;
    for(int i = 0; i < 5; i++)
        if(plants_type[i] != 0xff)
            b = true;
    if(!b) return false;
    for(int i = 0; i < 5; i++) {
        char s[20];
        GetWindowText(hZombieType[i], s, 20);
        std::string str(s);
        if(count(str.begin(), str.end(), ' ') == (int)str.length()) {
            zombies_type[i] = 0;
            continue;
        }
        int t = FindZombieInMap(str);
        if(t == -1) return false;
        zombies_type[i] = t;
    }
    b = false;
    for(int i = 0; i < 5; i++)
        if(zombies_type[i] != 0)
            b = true;
    if(!b) return false;
    for(int i = 0; i < 5; i++) {
        int t = GetIntFrom(hZombieTime[i]);
        if(t < 0) return false;
        zombies_time[i] = t;
    }
    for(int i = 0; i < 5; i++) {
        int t = GetIntFrom(hZombieCol[i]);
        if(t == 0) t = 6;
        if(zombies_type[i] == 5) {  // 小偷
            if(t < 1 || t > 5) return false;
        } else {
            if(t < 6 || t > 9) return false;
        }
        zombies_col[i] = t;
    }
    int t[5];
    for(int i = 0; i < 5; i++)
        plants_no[i] = t[i] = GetIntFrom(hPlantNo[i]) - 1;
    std::sort(t, t + 5);
    for(int i = 0; i < 5; i++)
        if(t[i] != i) return false;
    for(int i = 0; i < 5; i++)
        t[i] = plants_type[i];
    for(int i = 0; i < 5; i++) {
        plants_type[plants_no[i]] = t[i];
        plants_col[plants_no[i]] = i + 1;
    }
    return true;
}

bool Prepare(HWND hWnd, bool gameui) {
    hGameWindow = FindWindow("MainWindow", NULL);
    if(hGameWindow == NULL) {
        MessageBox(hWnd, "未找到游戏。", "提示", MB_OK | MB_ICONWARNING);
        return false;
    }
    DWORD proc_id;
    GetWindowThreadProcessId(hGameWindow, &proc_id);
    hGameProcess = OpenProcess(PROCESS_ALL_ACCESS, false, proc_id);
    if(gameui && read_memory<int>(0x6a9ec0, 0x7fc) != 3) {
        MessageBox(hWnd, "当前不在游戏界面。", "提示", MB_OK | MB_ICONWARNING);
        return false;
    }
    return true;
}

void StartTest() {
    write_memory<BYTE>(0xeb, 0x42b8f8);      // 禁止过关
    write_memory<BYTE>(1, 0x6a9ec0, 0x814);  // 免费种植
    write_memory<WORD>(0x00eb, 0x54eba8);    // 后台运行

    // 加速
    write_memory<BYTE>(0, 0x6a66f4);
    if(bSpeed) {
        write_memory<DWORD>(0xfff, 0x4526d3);
        write_memory<BYTE>(1, 0x6a9eab);
    }
    if(bSpeed || bHalfSpeed) {
        write_memory<DWORD>(1, 0x6a9ec0, 0x454);
    }

    write_memory<BYTE>(1, 0x700001);   // flag_start
    write_memory<BYTE>(1, 0x700000);   // flag
    write_memory<BYTE>(bDelayInf ? 1 : 0, 0x700003); // flag_delay_inf
    write_memory<DWORD>(0, 0x70000C);  // result
    write_memory<DWORD>(0, 0x700004);  // cur_count
    // write_memory<DWORD>(0, 0x6ffff8); // total_time
    write_memory<DWORD>(count_max, 0x6ffff4);
    write_memory<BYTE>(0xff, 0x6fffdb);
    write_memory<DWORD>(0xffffffff, 0x6fffdc);  // tmp_finished
    write_memory<DWORD>(key_plant_col - 1, 0x6ffff0);
    write_memory<BYTE>(0, 0x6fffda);  // current_zombie
    for(int i = 0; i < 5; i++) {
        write_memory<BYTE>(plants_type[i], 0x6fffe0 + i);
        write_memory<BYTE>(plants_col[i], 0x6fffbb + i);
    }
    int idx[5] = {0, 1, 2, 3, 4};
    std::sort(idx, idx + 5, [](int x, int y) -> bool {
        if(!zombies_type[x]) return false;
        if(!zombies_type[y]) return true;
        return zombies_time[x] < zombies_time[y];
    });
    for(zombie_cnt = 0; zombie_cnt < 5; zombie_cnt++)
        if(!zombies_type[idx[zombie_cnt]]) break;
    write_memory<BYTE>(zombie_cnt, 0x6fffef);
    for(int i = 0; i < zombie_cnt; i++) {
        write_memory<BYTE>(zombies_type[idx[i]], 0x6fffe5 + i);
        write_memory<WORD>(zombies_time[idx[i]], 0x6fffd0 + i * 2);
        write_memory<BYTE>(zombies_col[idx[i]], 0x6fffc0 + i);
    }

    start_time = read_memory<DWORD>(0x6a9ec0, 0x768, 0x5568);
    bRunning = true;
    if(!bNoInject) InjectCode(IDR_CODE);
    //	write_memory<DWORD>(0x001fd625, 0x453a67);
    write_memory<DWORD>(0x23b562e9, 0x415b29);  // 跳转
}
void EndTest() {
    //	write_memory<DWORD>(0x000fe735, 0x453a67);
    write_memory<DWORD>(0x5568be01, 0x415b29);  // 跳转
    write_memory<BYTE>(0x75, 0x42b8f8);
    write_memory<BYTE>(0, 0x6a9ec0, 0x814);
    bRunning = false;

    write_memory<DWORD>(1, 0x6a66f4);
    if(bSpeed)
        write_memory<BYTE>(0, 0x6a9eab);
    if(bSpeed || bHalfSpeed)
        write_memory<DWORD>(10, 0x6a9ec0, 0x454);

    write_memory<BYTE>(0, 0x700000);  // flag

    if(b5Test && bDLL && hDLL) {
        #pragma GCC diagnostic ignored "-Wcast-function-type"
        auto Result = (void (*)())GetProcAddress(hDLL, "Result");
        #pragma GCC diagnostic pop
        if(Result) Result();
        FreeLibrary(hDLL);
        hDLL = nullptr;
    }
}
