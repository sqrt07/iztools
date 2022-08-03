#include "iztools.h"
using namespace std;

void InjectCode(int resID);

extern HWND hCountInput, h5TestInput, hMjlockInput;

extern int start_time;
extern bool bSpeed, bHalfSpeed, bNoInject, bDLL, bDelay460;
bool bDLLSuccess;

PVOID pData, pCode, pCode2;

Args args;
HMODULE hDLL;
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
        
        int cnt = -1;
        for(int j = 0; j < (int)s.size(); j++) {
            if(s[j] != '+') {
                v1[i].push_back(string(""));
                v1[i][++cnt].push_back(s[j]);
            } else if(cnt == -1 || v1[i][cnt].length() != 1) return false;
            else if(v1[i][cnt][0] == '.') return false;
            else v1[i][cnt].push_back('+');
        }
        if(cnt != 4) return false;
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
            string s(v1[i][j].substr(0, 1));
            if(m_p.count(s) == 0) return false;
            args.PlantType[i][j] = m_p[s];
            if(v1[i][j].length() == 2)
                args.PlantType[i][j] += 0x80;
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
    args.CardTime = 0;
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
        if(args.ZombieTime[i] > args.CardTime)
            args.CardTime = args.ZombieTime[i];
    }
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

void LoadDLL(INJECTOR& Asm) {
    hDLL = LoadLibrary("script.dll");
    if(!hDLL) return;
    #pragma GCC diagnostic ignored "-Wcast-function-type"
    auto Script = (bool (*)(INJECTOR&, int, const DLLARGS&, DLLRET&))GetProcAddress(hDLL, "CallScript");
    #pragma GCC diagnostic pop
    if(!Script) { FreeLibrary(hDLL); hDLL = nullptr; return; }
    pfGETINT pfPlant = [](const string& s){ return m_p[s]; };
    pfGETINT pfZombie = [](const string& s){ return m_z[s]; };
    INJECTOR ScriptAsm;
    ScriptAsm.prepareForDLL();
    DLLARGS dllargs = { hGameProcess, pfPlant, pfZombie, args.ZombieTime };
    DLLRET ret = {};
    bDLLSuccess = Script(ScriptAsm, DLLVERSION, dllargs, ret);
    if(!bDLLSuccess) { FreeLibrary(hDLL); hDLL = nullptr; return; }
    Asm.mov(EAX, p_myclock)
        .cmp(EAX, 0ul)
        .if_jmp(jne, INJECTOR().mov(ECX, (DWORD)(ret.data_pos - p_eventflag))
                            .mov(EAX, -1)
                            .mov(EDI, (DWORD)p_eventflag)
                            .repe().stosd())
        .add(ScriptAsm);
}

inline initializer_list<DWORD> jmp_list_f = {
    0x524b97,  // 0.23~0.37
    0x524c3d,  // 0.66~0.68
    0x524c14,  // 0.79~0.81
    0x53347a,  // 僵尸死亡动画
};
inline initializer_list<DWORD> jmp_list = {
    0x45dee2,  // 0~150
    0x45e66f,  // 小喷横向偏移
    0x45f1e5,  // 玉米/黄油
    0x45f8ba,  // 136~150
    0x5234fe,  // 舞王滑步时长（300~311）
    0x522a26,  // 蹦极高度
    0x52b53b,  // 濒死僵尸随机减血（1/5概率减1）
    0x52f3d4,  // 普僵顺拐/正常
    0x5334a2,  // 僵尸超长死亡（1/100）
    0x42a7fa, 0x42af4b, 0x42afa6, 0x42b019, 0x42b05b, 0x42b0e1, // 随机生成阵型
    0x407eef, 0x407efb, 0x407f0e,
    0x40dabb
};
void WriteRndJmp(DWORD pjmp) {
    for(DWORD ptr : jmp_list_f)
        write_memory<DWORD>(pjmp - ptr - 5, ptr + 1);
    for(DWORD ptr : jmp_list)
        write_memory<DWORD>(pjmp - ptr, ptr + 1);
}
void ClearRndJmp() {
    for(DWORD ptr : jmp_list_f)
        write_memory(0x511cb0 - ptr - 5, ptr + 1);
    for(DWORD ptr : jmp_list)
        write_memory(0x5af400 - ptr - 5, ptr + 1);
}

void InjectRndRec() {
    INJECTOR Asm, Asm2;
    // rnd_float
    Asm.add_word(0x058f).add_dword(0x6ffffc) // pop [6ffffc]
        .mov((DWORD*)0x700018, EDI)
        .call(0x511cb0) // r_rnd_jieduan_f
        .add_word(0x35ff).add_dword(0x6ffffc) // push [6ffffc]
        .cmp((BYTE*)0x70001c, 1)
        .if_jmp(EQUAL, INJECTOR()
                           .mov(EDI, (DWORD*)0x700014)
                           .add_word(0x17d9) // fst [edi]
                           .add(EDI, 4)
                           .mov((DWORD*)0x700014, EDI))
        .cmp((BYTE*)0x70001c, 2)
        .if_jmp(EQUAL, INJECTOR()
                           .mov(EDI, (DWORD*)0x700020)
                           .cmp(EDI, (DWORD*)0x700014)
                           .if_jmp(EQUAL, INJECTOR()
                                .mov((BYTE*)0x70004c, (BYTE)TEXTTYPE::END)
                                .mov((BYTE*)0x70001c, 0))
                           .if_jmp(BELOW, INJECTOR()
                                .add_word(0xd8dd) // fstp st(0)
                                .add_word(0x07d9)) // fld [edi]
                           .add(EDI, 4)
                           .mov((DWORD*)0x700020, EDI))
        .mov(EDI, (DWORD*)0x700018);
    
    // rnd_int
    Asm2.mov((DWORD*)0x700018, EDI)
        .call(0x5af400)
        .cmp((BYTE*)0x70001c, 1)
        .if_jmp(EQUAL, INJECTOR()
                           .mov(EDI, (DWORD*)0x700014)
                           .mov(PEDI, EAX)
                           .add(EDI, 4)
                           .mov((DWORD*)0x700014, EDI))
        .cmp((BYTE*)0x70001c, 2)
        .if_jmp(EQUAL, INJECTOR()
                           .mov(EDI, (DWORD*)0x700020)
                           .cmp(EDI, (DWORD*)0x700014)
                           .if_jmp(BELOW, INJECTOR()
                                .mov(EAX, PEDI))
                           .if_jmp(EQUAL, INJECTOR()
                                .mov((BYTE*)0x70004c, (BYTE)TEXTTYPE::END)
                                .mov((BYTE*)0x70001c, 0))
                           .add(EDI, 4)
                           .mov((DWORD*)0x700020, EDI))
        .mov(EDI, (DWORD*)0x700018);

    pCode = AllocMemory(Asm.len() + 1);
    pCode2 = AllocMemory(Asm2.len() + 1);
    Asm.write(pCode);
    Asm2.write(pCode2);

    const DWORD pjmp = 0x401872;
    Asm.clear();
    Asm.add_byte(0xe9).add_dword((DWORD)pCode - pjmp - 5);
    Asm.add_byte(0xe9).add_dword((DWORD)pCode2 - pjmp - 10);
    Asm.write((void*)pjmp);

    WriteRndJmp(pjmp);
}

bool Start5Test() {
    INJECTOR Asm, Asm2;
    gp.init();
    if(args.ZombieCnt > 0) { // 僵尸编号设置
        int idx[21] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
        stable_sort(idx, idx + args.ZombieCnt, [](int i, int j) {
            return args.ZombieTime[i] < args.ZombieTime[j];
        }); // idx[i]: 第i个放置的僵尸的栈位
        Asm2.mov(gp.zombies.cnt_max(), args.ZombieCnt)
            .mov(gp.zombies.next(), idx[0]);
        for(int i = 0; i < args.ZombieCnt; i++)
            Asm2.mov(gp.zombies[idx[i]].next(), idx[i + 1]);
        Asm.cmp(gp.myclock, 0)
            .if_jmp(jne, Asm2);
    }
    for(int i = 0; i < args.ZombieCnt; i++) {
        if(bDelay460) args.ZombieTime[i] += 460;
        Asm.cmp(gp.myclock, args.ZombieTime[i])
            .if_jmp(jne, INJECTOR()
                             .use_card(args.ZombieRow[i], args.ZombieCol[i], args.ZombieType[i]));
    }
    if(bDelay460) args.CardTime += 460;
    Asm.write((void*)0x651800);
    Asm.clear();
    
    bDLLSuccess = false;
    if(bDLL) {
        if(hDLL) {
            MessageBox(hWnd, "DLL未退出", "提示", MB_OK | MB_ICONINFORMATION);
            return false;
        }
        LoadDLL(Asm);
    }
    
    Asm.write((void*)0x651b00);  // ret

    pData = AllocMemory(120 * 1024);
    write_memory<void*>(pData, 0x700020); // p_data
    InjectRndRec();
    
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

    write_memory<BYTE>(0, 0x70001c);            // flag_data
    write_memory<BYTE>(0, 0x70001d);            // flag_remember
    write_memory<BYTE>(1, 0x700001);            // flag_start
    write_memory<BYTE>(1, 0x700000);            // flag
    write_memory<BYTE>(0, 0x700002);            // flag_state
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
    return true;
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