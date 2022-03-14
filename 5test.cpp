#include "iztools.h"
using namespace std;

void InjectCode(int resID);

extern HWND hCountInput, h5TestInput, hMjlockInput;

extern int start_time;
extern bool bSpeed, bHalfSpeed, bNoInject, bDLL;

Args args;
void divide_text(const string& s, vector<string>& v) {
    static const string spaces = " \t\r\n";
    size_t i = 0;
    while((i = s.find_first_not_of(spaces, i)) != string::npos) {
        auto begin = s.begin() + i;
        i = s.find_first_of(spaces, i);
        if(i == string::npos)
            i = s.length();
        auto end = s.begin() + i;
        string str(begin, end);
        v.push_back(str);
    }
}
bool isInteger(const string& s) {
    for(const char ch : s)
        if(!isdigit(ch))
            return false;
    return true;
}
POINT read_point(const string& s) {
    int t = s.find('-');
    if(t == -1) return {0, 0};
    string s1(s.begin(), s.begin() + t);
    string s2(s.begin() + t + 1, s.end());
    if(!isInteger(s1) || !isInteger(s2))
        return {0, 0};
    return {atoi(s1.c_str()), atoi(s2.c_str())};
}
bool ReadTestStr(const char* input_str) {
    istringstream fin(input_str);
    string sPlantType[5], sKeyPos, sZombieType, sZombieTime, sZombiePos, sMjlock;
    fin >> args.Total >> args.Mjlock;
    if(fin.get() == '\r') fin.get();
    getline(fin, sKeyPos);
    for(int i = 0; i < 5; i++)
        getline(fin, sPlantType[i]);
    getline(fin, sZombieType);
    getline(fin, sZombieTime);
    getline(fin, sZombiePos);
    getline(fin, sMjlock);
    if(args.Total <= 0 || (args.Mjlock != (unsigned)-1 && args.Mjlock >= 460))
        return false;
    vector<string> v0, v1[5], v2, v3, v4;
    divide_text(sKeyPos, v0);
    for(int i = 0; i < 5; i++) {
        string s;
        for(char x : sPlantType[i])
            if(x != ' ' && x != '\t' && x != '\r')
                s.push_back(x);
        if(s.length() != 5) return false;
        for(int j = 0; j < 5; j++) {
            v1[i].push_back(string(""));
            v1[i][j].push_back(s[j]);
        }
    }
    divide_text(sZombieType, v2);
    divide_text(sZombieTime, v3);
    divide_text(sZombiePos, v4);
    args.ZombieCnt = v2.size();
    if(args.ZombieCnt > 20) return false;
    if(args.ZombieCnt != v3.size()) return false;
    if(args.ZombieCnt != v4.size()) return false;
    for(int i = 0; i < 5; i++)
        for(int j = 0; j < 5; j++) {
            if(m_p.count(v1[i][j]) == 0) return false;
            args.PlantType[i][j] = m_p[v1[i][j]];
        }
    args.KeyCnt = v0.size();
    if(args.KeyCnt == 0 || args.KeyCnt > 10) return false;
    for(int i = 0; i < args.KeyCnt; i++) {
        POINT p = read_point(v0[i]);
        if(p.x <= 0 || p.y < 0 || p.x > 5 || p.y > 5) return false;
        if(p.y != 0 && args.PlantType[p.x - 1][p.y - 1] == (BYTE)-1) return false;
        args.KeyRow[i] = p.x;
        args.KeyCol[i] = p.y;
    }
    for(int i = 0; i < args.ZombieCnt; i++) {
        if(m_z.count(v2[i]) == 0) return false;
        args.ZombieType[i] = m_z[v2[i]];
        if(!isInteger(v3[i])) return false;
        args.ZombieTime[i] = atoi(v3[i].c_str());
        POINT p = read_point(v4[i]);
        if(p.x < 1 || p.x > 5) return false;
        if(args.ZombieType[i] == 5) {  // 小偷
            if(p.y < 1 || p.y > 5) return false;
        } else {
            if(p.y < 6 || p.y > 9) return false;
        }
        args.ZombieRow[i] = p.x;
        args.ZombieCol[i] = p.y;
    }
    int idx[20] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
    stable_sort(idx, idx + args.ZombieCnt, [](int i, int j) -> bool {
        return args.ZombieTime[i] < args.ZombieTime[j];
    });
    Args _args = args;
    for(int i = 0; i < args.ZombieCnt; i++) {
        args.ZombieRow[i] = _args.ZombieRow[idx[i]];
        args.ZombieCol[i] = _args.ZombieCol[idx[i]];
        args.ZombieType[i] = _args.ZombieType[idx[i]];
        args.ZombieTime[i] = _args.ZombieTime[idx[i]];
    }
    args.CardTime = args.ZombieTime[args.ZombieCnt - 1];
    for(int i = 0; i < args.KeyCnt; i++) {
        args.KeyRow[i]--;
        args.KeyCol[i]--;
    }
    args.NoMj = 1;
    for(int i = 0; i < args.ZombieCnt; i++)
        if(args.ZombieType[i] == m_z["ww"])
            args.NoMj = 0;
    return true;
}

void Start5Test() {
    INJECTOR Asm;
    Asm.mov_p(EAX, p_myclock);
    for(int i = 0; i < args.ZombieCnt; i++) {
        Asm.cmp(EAX, args.ZombieTime[i])
            .if_jmp(jne, INJECTOR()
                             .use_card(args.ZombieRow[i], args.ZombieCol[i], args.ZombieType[i]));
    }
    Asm.write((void*)0x651800);
    Asm.clear();

    if(bDLL) {
        HMODULE hDLL = LoadLibrary("script.dll");
        if(hDLL) {
            #pragma GCC diagnostic ignored "-Wcast-function-type"
            auto Script = (void (*)(INJECTOR&, HANDLE, pfGETINT, pfGETINT))GetProcAddress(hDLL, "CallScript");
            #pragma GCC diagnostic pop
            if(Script) {
                INJECTOR Asm2;
                Asm2.mov(EAX, -1);
                for(DWORD* p = (DWORD*)0x700100; p < (DWORD*)0x700128; p++)
                    Asm2.mov_p_r(p, EAX);
                Asm.mov_p(EAX, p_myclock)
                    .cmp(EAX, 0)
                    .if_jmp(jne, Asm2);
                Asm.prepareForDLL();

                pfGETINT pfPlant = [](const std::string& s){ return m_p[s]; };
                pfGETINT pfZombie = [](const std::string& s){ return m_z[s]; };
                Script(Asm, hGameProcess, pfPlant, pfZombie);
            }
            FreeLibrary(hDLL);
        }
    }

    Asm.write((void*)0x651b00);  // ret

    // 加速
    write_memory<BYTE>(0, 0x6a66f4);
    if(bSpeed) {
        write_memory<DWORD>(0xfff, 0x4526d3);
        write_memory<BYTE>(1, 0x6a9eab);
    }
    if(bSpeed || bHalfSpeed) {
        write_memory<DWORD>(1, 0x6a9ec0, 0x454);
    }

    write_memory<BYTE>(0xeb, 0x42b8f8);      // 禁止过关
    write_memory<BYTE>(1, 0x6a9ec0, 0x814);  // 免费种植
    write_memory<WORD>(0x00eb, 0x54eba8);    // 后台运行

    write_memory<BYTE>(1, 0x700001);            // flag_start
    write_memory<BYTE>(1, 0x700000);            // flag
    write_memory<DWORD>(0, 0x70000C);           // result
    write_memory<DWORD>(0, 0x700004);           // cur_count
    write_memory<DWORD>(args.Total, 0x6ffff4);  // count_max

    write_memory<BYTE>(args.NoMj, 0x6fff7f);       // no_mj
    write_memory<DWORD>(args.Mjlock, 0x6fff80);    // mjlock
    write_memory<DWORD>(args.KeyCnt, 0x6fff89);    // key_cnt
    write_memory<DWORD>(args.CardTime, 0x6fff8a);  // card_time
    WriteProcessMemory(hGameProcess, (void*)0x6fffa2, (void*)args.PlantType, 25, NULL);
    WriteProcessMemory(hGameProcess, (void*)0x6fff8e, (void*)args.KeyRow, 10, NULL);
    WriteProcessMemory(hGameProcess, (void*)0x6fff98, (void*)args.KeyCol, 10, NULL);

    start_time = read_memory<DWORD>(0x6a9ec0, 0x768, 0x5568);
    bRunning = true;
    if(!bNoInject) InjectCode(IDR_CODE5);
    write_memory<DWORD>(0x23b562e9, 0x415b29);  // 跳转
}

void GetString(char* s) {
    char s1[10] = "", s2[10] = "", s3[1024] = "";
    GetWindowText(hCountInput, s1, 10);
    GetWindowText(hMjlockInput, s2, 10);
    GetWindowText(h5TestInput, s3, 1024);
    if(s2[0] == '\0') strcpy(s2, "-1");
    sprintf(s, "%s %s\n%s", s1, s2, s3);
}
void SetString(const char* s) {
    if(!ReadTestStr(s)) {
        MessageBox(hWnd, "输入不合法。", "提示", MB_OK | MB_ICONWARNING);
        return;
    }
    char s1[10], s2[10];
    istringstream fin(s);
    fin >> s1 >> s2;
    if(fin.get() == '\r') fin.get();
    string s3;
    while(!fin.eof()) {
        char ch = fin.get();
        if(ch == EOF) break;
        if(ch == '\n' && (s3.empty() || *s3.rbegin() != '\r'))
            s3.push_back('\r');
        s3.push_back(ch);
    }
    SetWindowText(hCountInput, s1);
    if(strcmp(s2, "-1"))
        SetWindowText(hMjlockInput, s2);
    else
        SetWindowText(hMjlockInput, "");
    SetWindowText(h5TestInput, s3.c_str());
}