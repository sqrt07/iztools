#include <windows.h>
#include <map>
#include <string>
#include <vector>
#include "memory.h"

static_assert(sizeof(void*) == 4, "请使用32位编译");

typedef int (*pfGETINT)(const std::string&);

enum REG { EAX, EBX, ECX, EDX, ESI, EDI };
enum CONDJMP : BYTE { jae = 3, je = 4, jne = 5, jbe = 6,
                      BELOW = 3, NEQUAL = 4, EQUAL = 5, ABOVE = 6 };

class GAMEPTR {
   public:
    int base, obj, plant, zombie, bullet, prog, clock;
    DWORD* plant_cnt;
    DWORD* zombie_cnt;
    void init() {
        base = read_memory<DWORD>(0x6a9ec0);
        obj = read_memory<DWORD>(base + 0x768);
        plant = read_memory<DWORD>(obj + 0xac);
        zombie = read_memory<DWORD>(obj + 0x90);
        bullet = read_memory<DWORD>(obj + 0xc8);
        prog = read_memory<DWORD>(obj + 0x160);
        clock = obj + 0x5568;
        plant_cnt = (DWORD*)(obj + 0xbc);
        zombie_cnt = (DWORD*)(obj + 0xa0);
    }
    int get_plant(int idx) const { return plant + 0x14c * idx; }
    int get_zombie(int idx) const { return zombie + 0x15c * idx; }
};

#define p_data (DWORD*)0x700100
#define p_myclock (DWORD*)0x700010

class COMPARE;
class INJECTOR {
    BYTE code[0x400];
    int pos;

   public:
    INJECTOR() : pos(0) {}
    int len() const { return pos; }
    void clear() { pos = 0; }
    void add_byte(BYTE c) {
        code[pos] = c;
        pos += 1;
    }
    void add_word(WORD c) {
        (WORD&)code[pos] = c;
        pos += 2;
    }
    void add_dword(DWORD c) {
        (DWORD&)code[pos] = c;
        pos += 4;
    }
    void add(const INJECTOR& Asm) {
        memcpy(code + pos, Asm.code, Asm.pos);
        pos += Asm.pos;
    }
    void write(void* addr = (void*)0x651090);
    INJECTOR& ret() {
        add_byte(0xc3);
        return *this;
    }
    INJECTOR& push(DWORD c) {
        add_byte(0x68);
        add_dword(c);
        return *this;
    }
    INJECTOR& push(REG r) {
        BYTE t[] = {0x50, 0x53, 0x51, 0x52, 0x56, 0x57};
        add_byte(t[r]);
        return *this;
    }
    INJECTOR& call(DWORD c) {
        mov(EDI, c);
        add_word(0xd7ff);  // call edi
        return *this;
    }
    INJECTOR& mov(REG r, DWORD c) {  // mov r, c
        BYTE t[] = {0xb8, 0xbb, 0xb9, 0xba, 0xbe, 0xbf};
        add_byte(t[r]);
        add_dword(c);
        return *this;
    }
    INJECTOR& mov_p(REG r, DWORD* p) {  // mov r, dword ptr[p]
        BYTE t[] = {0xa1, 0x1d, 0x0d, 0x15, 0x35, 0x3d};
        if(!(r == EAX)) add_byte(0x8b);
        add_byte(t[r]);
        add_dword((DWORD)p);
        return *this;
    }
    INJECTOR& mov_p_r(DWORD* p, REG r) {  // mov dword ptr[p], r
        BYTE t[] = {0xa3, 0x1d, 0x0d, 0x15, 0x35, 0x3d};
        if(!(r == EAX)) add_byte(0x89);
        add_byte(t[r]);
        add_dword((DWORD)p);
        return *this;
    }
    INJECTOR& add(REG r, DWORD c) {  // add r, c
        BYTE t[] = {0x05, 0xc3, 0xc1, 0xc2, 0xc6, 0xc7};
        if(!(r == EAX)) add_byte(0x81);
        add_byte(t[r]);
        add_dword(c);
        return *this;
    }
    INJECTOR& cmp(REG r, DWORD c) {  // cmp r, c
        BYTE t[] = {0x3d, 0xfb, 0xf9, 0xfa, 0xfe, 0xff};
        if(!(r == EAX)) add_byte(0x81);
        add_byte(t[r]);
        add_dword(c);
        return *this;
    }
    INJECTOR& cmp_p(REG r, DWORD* p) {  // cmp r, dword ptr[p]
        BYTE t[] = {0x05, 0x1d, 0x0d, 0x15, 0x35, 0x3d};
        add_byte(0x3b);
        add_byte(t[r]);
        add_dword((DWORD)p);
        return *this;
    }
    INJECTOR& cmp_p_c(DWORD* p, DWORD c) {  // cmp dword ptr[p], c
        add_word(0x3d81);
        add_dword((DWORD)p);
        add_dword(c);
        return *this;
    }
    INJECTOR& cmp_p_c(BYTE* p, BYTE c) {  // cmp byte ptr[p], c
        add_word(0x3d80);
        add_dword((DWORD)p);
        add_byte(c);
        return *this;
    }
    INJECTOR& cond_jmp(CONDJMP code, DWORD addr) {
        if((BYTE)addr == addr) {
            add_byte(0x70 + code);
            add_byte((BYTE)addr);
        } else {
            add_byte(0x0f);
            add_byte(0x80 + code);
            add_dword(addr);
        }
        return *this;
    }
    INJECTOR& jmp(DWORD addr) {
        if((BYTE)addr == addr) {
            add_byte(0xeb);
            add_byte((BYTE)addr);
        } else {
            add_byte(0xe9);
            add_dword(addr);
        }
        return *this;
    }
    INJECTOR& if_jmp(CONDJMP code, const INJECTOR& Asm) {
        cond_jmp(code, Asm.pos);
        add(Asm);
        return *this;
    }

    // 添加只执行一次的事件（第一次触发）
    // cond:  触发条件（用&&连接要求同时满足的多个条件）
    // delay: 触发后延迟的时间
    // Asm:   执行的代码
    INJECTOR& event1(const COMPARE& cond, int delay, const INJECTOR& Asm);

    // 添加只执行一次的事件（最后一次触发：等待过程中如果再次触发条件，重新delay）
    // cond:  触发条件（用&&连接要求同时满足的多个条件）
    // delay: 触发后延迟的时间
    // Asm:   执行的代码
    // cond2: 二次判断的条件（可省略）：执行代码时检查是否满足条件，若不满足则取消执行，重新等待
    INJECTOR& event2(const COMPARE& cond, int delay, const INJECTOR& Asm, const COMPARE& cond2);

    // row和col从1开始；type参见m_z
    INJECTOR& use_card(int row, int col, int type) {
        return push(col).push(row).push(type).call(0x6510b3);
    }

    void put_plant(int row, int col, int type);
    void del_plant(int i);
    void del_zombie(int i);
    void del_bullet(int i);
    void clear_plants();
    void clear_zombies();
    void clear_bullets();
};

class COMPARE {
    friend class INJECTOR;
    std::vector<std::pair<INJECTOR, CONDJMP>> vec;

   public:
    // ptr指向的数据与c比较
    COMPARE(DWORD* ptr, CONDJMP cond, DWORD c) {
        vec.push_back({INJECTOR().cmp_p_c(ptr, c), cond});
    }

    // ptr指向的数据与c比较
    COMPARE(BYTE* ptr, CONDJMP cond, BYTE c) {
        vec.push_back({INJECTOR().cmp_p_c(ptr, c), cond});
    }

    // 永远为真
    COMPARE() {}

    COMPARE operator&&(const COMPARE& eq2) const {
        COMPARE eq1(*this);
        eq1.vec.insert(eq1.vec.end(), eq2.vec.begin(), eq2.vec.end());
        return eq1;
    }
    INJECTOR if_jmp(const INJECTOR& Asm) const {
        INJECTOR Asm2(Asm);
        for(auto it = vec.rbegin(); it != vec.rend(); ++it) {
            INJECTOR Asm3(Asm2);
            Asm2.clear();
            Asm2.add(it->first);
            Asm2.if_jmp(it->second, Asm3);
        }
        return Asm2;
    }
};

#ifdef SCRIPT_DLL
static DWORD* data_pos;

inline INJECTOR& INJECTOR::event1(const COMPARE& cond, int delay, const INJECTOR& Asm) {
    COMPARE comp = COMPARE(data_pos, EQUAL, -1) && cond;
    add(comp.if_jmp(INJECTOR()
                        .mov_p(EAX, p_myclock)
                        .add(EAX, delay)
                        .mov_p_r(data_pos, EAX)));
    mov_p(EAX, data_pos);
    cmp_p(EAX, p_myclock);
    if_jmp(jne, Asm);
    data_pos++;
    return *this;
}
inline INJECTOR& INJECTOR::event2(const COMPARE& cond, int delay, const INJECTOR& Asm,
                                  const COMPARE& cond2 = COMPARE()) {
    COMPARE comp = COMPARE(data_pos, NEQUAL, -2) && cond;
    add(comp.if_jmp(INJECTOR()
                        .mov_p(EAX, p_myclock)
                        .add(EAX, delay)
                        .mov_p_r(data_pos, EAX)));
    mov_p(EAX, data_pos);
    cmp_p(EAX, p_myclock);
    if_jmp(jne, cond2.if_jmp(INJECTOR(Asm)
                                 .mov(EAX, -2)
                                 .mov_p_r(data_pos, EAX)));
    data_pos++;
    return *this;
}
#endif