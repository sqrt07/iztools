#include <windows.h>
#include <map>
#include <string>
#include <vector>
#include "memory.h"

static_assert(sizeof(void*) == 4, "请使用32位编译");

typedef int (*pfGETINT)(const std::string&);

enum REG { EAX, EBX, ECX, EDX, ESI, EDI };
enum CONDJMP : BYTE { jb  = 2, NBELOW = 2,
                      jnb = 3,  BELOW = 3,
                      je  = 4, NEQUAL = 4,
                      jne = 5,  EQUAL = 5,
                      jna = 6,  ABOVE = 6,
                      ja  = 7, NABOVE = 7 };


inline DWORD* const p_data = (DWORD*)0x700100;
inline DWORD* const p_myclock = (DWORD*)0x700010;
inline DWORD* const p_mydata = (DWORD*)0x700900;

class COMPARE;
class INJECTOR {
    std::vector<BYTE> code;
    inline static const BYTE treg[] = {0, 3, 1, 2, 6, 7};

   public:
    int len() const { return code.size(); }
    void clear() { code.clear(); }
    void add_byte(BYTE c) { code.push_back(c); }
    void add_word(WORD c) {
        add_byte(c);
        add_byte(c >> 8);
    }
    void add_dword(DWORD c) {
        add_word(c);
        add_word(c >> 16);
    }
    void add_ptr(void* p) {
        add_dword((DWORD)p);
    }
    void add(const INJECTOR& Asm) {
        code.insert(code.end(), Asm.code.begin(), Asm.code.end());
    }
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
        add_byte(0x50 + treg[r]);
        return *this;
    }
    INJECTOR& call(DWORD c) {
        mov(EDI, c);
        add_word(0xd7ff);  // call edi
        return *this;
    }
    INJECTOR& mov(REG r, REG r2) {  // mov r, r2
        add_byte(0x8b);
        add_byte(0xc0 + treg[r] * 8 + treg[r2]);
        return *this;
    }
    INJECTOR& mov(REG r, DWORD c) {  // mov r, c
        add_byte(0xb8 + treg[r]);
        add_dword(c);
        return *this;
    }
    INJECTOR& mov(REG r, DWORD* p) {  // mov r, dword ptr[p]
        if(r == EAX) add_byte(0xa1);
        else {
            add_byte(0x8b);
            add_byte(0x05 + treg[r] * 8);
        }
        add_ptr(p);
        return *this;
    }
    INJECTOR& mov(DWORD* p, REG r) {  // mov dword ptr[p], r
        if(r == EAX) add_byte(0xa3);
        else {
            add_byte(0x89);
            add_byte(0x05 + treg[r] * 8);
        }
        add_ptr(p);
        return *this;
    }
    INJECTOR& mov(DWORD* p, DWORD c) {  // mov dword ptr[p], c
        add_word(0x05c7);
        add_ptr(p);
        add_dword(c);
        return *this;
    }
    INJECTOR& mov(WORD* p, WORD c) {  // mov word ptr[p], c
        add_byte(0x66);
        add_word(0x05c7);
        add_ptr(p);
        add_word(c);
        return *this;
    }
    INJECTOR& mov(BYTE* p, BYTE c) {  // mov byte ptr[p], c
        add_word(0x05c6);
        add_ptr(p);
        add_byte(c);
        return *this;
    }
    INJECTOR& add(REG r, REG r2) {  // add r, r2
        add_byte(0x01);
        add_byte(0xc0 + treg[r2] * 8 + treg[r]);
        return *this;
    }
    INJECTOR& add(REG r, DWORD c) {  // add r, c
        if(r == EAX) add_byte(0x05);
        else {
            add_byte(0x81);
            add_byte(0xc0 + treg[r]);
        }
        add_dword(c);
        return *this;
    }
    INJECTOR& sub(REG r, DWORD c) {  // sub r, c
        if(r == EAX) add_byte(0x2d);
        else {
            add_byte(0x81);
            add_byte(0xe8 + treg[r]);
        }
        add_dword(c);
        return *this;
    }
    INJECTOR& div(REG r) {  // div r
        add_byte(0xf7);
        add_byte(0xf0 + treg[r]);
        return *this;
    }
    INJECTOR& cmp(REG r, DWORD c) {  // cmp r, c
        if(r == EAX) add_byte(0x3d);
        else {
            add_byte(0x81);
            add_byte(0xf8 + treg[r]);
        }
        add_dword(c);
        return *this;
    }
    INJECTOR& cmp(REG r, DWORD* p) {  // cmp r, dword ptr[p]
        add_byte(0x3b);
        add_byte(0x05 + treg[r] * 8);
        add_ptr(p);
        return *this;
    }
    INJECTOR& cmp(DWORD* p, DWORD c) {  // cmp dword ptr[p], c
        add_word(0x3d81);
        add_ptr(p);
        add_dword(c);
        return *this;
    }
    INJECTOR& cmp(WORD* p, WORD c) {  // cmp word ptr[p], c
        add_byte(0x66);
        add_word(0x3d81);
        add_ptr(p);
        add_word(c);
        return *this;
    }
    INJECTOR& cmp(BYTE* p, BYTE c) {  // cmp byte ptr[p], c
        add_word(0x3d80);
        add_ptr(p);
        add_byte(c);
        return *this;
    }
    INJECTOR& repe() { add_byte(0xf3); return *this; }
    INJECTOR& stosd() { add_byte(0xab); return *this; }
    INJECTOR& quit() {
        mov(EAX, (DWORD*)0x700004);
        add(EAX, 1);
        mov((DWORD*)0x6ffff4, EAX);
        return *this;
    }
    INJECTOR& win() {
        mov((BYTE*)0x700002, 2);
        return *this;
    }
    INJECTOR& lose() {
        mov((BYTE*)0x700002, 1);
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
        cond_jmp(code, Asm.len());
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

    // row和col从1开始；type使用Zombie("xx")
    INJECTOR& use_card(int row, int col, int type) {
        return push(col).push(row).push(type).call(0x6510b3);
    }

    // 下面这些函数是给“调整行”准备的，请不要使用
    void put_plant(int row, int col, int type);
    void del_plant(int i);
    void del_zombie(int i);
    void del_bullet(int i);
    void clear_plants();
    void clear_zombies();
    void clear_bullets();

    // 下面这些函数是内部使用的
    void write(void* addr = (void*)0x651090);
    void prepareForDLL() {
        code.reserve(10000);
    }
};

class COMPARE {
    friend class INJECTOR;
    std::vector<std::pair<INJECTOR, CONDJMP>> vec;

   public:
    // ptr指向的数据与c比较
    COMPARE(DWORD* ptr, CONDJMP cond, DWORD c) {
        vec.push_back({INJECTOR().cmp(ptr, c), cond});
    }
    COMPARE(WORD* ptr, CONDJMP cond, WORD c) {
        vec.push_back({INJECTOR().cmp(ptr, c), cond});
    }
    COMPARE(BYTE* ptr, CONDJMP cond, BYTE c) {
        vec.push_back({INJECTOR().cmp(ptr, c), cond});
    }

    // ptr指向的数据ptr2指向的数据比较
    COMPARE(DWORD* ptr, CONDJMP cond, DWORD* ptr2) {
        vec.push_back({INJECTOR().mov(EAX, ptr).cmp(EAX, ptr2), cond});
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

template<typename T>
class POINTER {
    T* ptr;
   public:
    POINTER(T* ptr) : ptr(ptr) {}
    T* p() const { return ptr; }
    T read() const { return read_memory(ptr); }
    void write(T x) const { return write_memory(x, ptr); }
    POINTER<T> operator+(int x) const { return ptr + x; }
    POINTER<T> operator-(int x) const { return ptr - x; }
    POINTER<T> operator-(POINTER<T> p2) const { return ptr - p2.ptr; }
    COMPARE operator==(POINTER<T> p2) { return {ptr,  EQUAL, p2.ptr}; }
    COMPARE operator!=(POINTER<T> p2) { return {ptr, NEQUAL, p2.ptr}; }
    COMPARE operator< (POINTER<T> p2) { return {ptr,  BELOW, p2.ptr}; }
    COMPARE operator>=(POINTER<T> p2) { return {ptr, NBELOW, p2.ptr}; }
    COMPARE operator> (POINTER<T> p2) { return {ptr,  ABOVE, p2.ptr}; }
    COMPARE operator<=(POINTER<T> p2) { return {ptr, NABOVE, p2.ptr}; }
    COMPARE operator==(T x) { return {ptr,  EQUAL, x}; }
    COMPARE operator!=(T x) { return {ptr, NEQUAL, x}; }
    COMPARE operator< (T x) { return {ptr,  BELOW, x}; }
    COMPARE operator>=(T x) { return {ptr, NBELOW, x}; }
    COMPARE operator> (T x) { return {ptr,  ABOVE, x}; }
    COMPARE operator<=(T x) { return {ptr, NABOVE, x}; }
};

class GAMEPTR {
   public:
    class ITEMBASE {
        int ptr;
       public:
        ITEMBASE(int ptr = 0) : ptr(ptr) {}
        int p() const { return ptr; }
        template<typename T = DWORD>
        T* get(int offset) const { return (T*)(ptr + offset); }
        DWORD* operator[](int offset) const { return get(offset); }
    };
    template<typename ITEM, int size>
    class ITEMSBASE {
        int ptr;
        POINTER<DWORD> pcnt;
       public:
        ITEMSBASE(int ptr = 0, int pcnt = 0) : ptr(ptr), pcnt((DWORD*)pcnt) {}
        int p() const { return ptr; }
        POINTER<DWORD> cnt() const { return pcnt; }
        POINTER<DWORD> cnt_max() const { return pcnt - 3; }
        ITEM operator[](int idx) const { return {ptr + idx * size}; }
    };
    struct PLANT : ITEMBASE {
        using ITEMBASE::ITEMBASE, ITEMBASE::operator[];
        // 以下函数命名与CVP保持一致
        POINTER<DWORD> col() const { return get(0x28) + 1; } // 从1开始
        POINTER<DWORD> hp() const { return get(0x40); }
        POINTER<DWORD> attackCountdown() const { return get(0x58); }
        POINTER<DWORD> row() const { return get(0x88) + 1; } // 从1开始
        POINTER<DWORD> shootCountdown() const { return get(0x90); }
        POINTER<BYTE> isVanished() const { return get<BYTE>(0x141); }
    };
    struct ZOMBIE : ITEMBASE {
        using ITEMBASE::ITEMBASE, ITEMBASE::operator[];
        // 以下函数命名与CVP保持一致
        POINTER<DWORD> state() const { return get(0x28); }
        POINTER<DWORD> hp() const { return get(0xc8); }
        POINTER<DWORD> helmetHp() const { return get(0xd0); }
        POINTER<DWORD> shieldHp() const { return get(0xdc); }
        POINTER<DWORD> slowCountdown() const { return get(0xac); }
        POINTER<BYTE> haveHead() const { return get<BYTE>(0xba); }
    };
    int base, obj, plant, zombie, bullet, prog;
    PDWORD clock, mjclock;
    ITEMSBASE<PLANT, 0x14c> plants;
    ITEMSBASE<ZOMBIE, 0x15c> zombies;
    void init() {
        base = read_memory<int>(0x6a9ec0);
        obj = read_memory<int>(base + 0x768);
        plant = read_memory<int>(obj + 0xac);
        zombie = read_memory<int>(obj + 0x90);
        bullet = read_memory<int>(obj + 0xc8);
        prog = read_memory<int>(obj + 0x160);
        clock = (DWORD*)(obj + 0x5568);
        mjclock = (DWORD*)(base + 0x838);
        plants = {plant, obj + 0xbc};
        zombies = {zombie, obj + 0xa0};
    }
    // PLANT get_plant(int row, int col) const { // 它现在是错的
    //     int cnt_max = plants.cnt_max().read();
    //     for(int idx = 0; idx < cnt_max; idx++) {
    //         PLANT p = plants[idx];
    //         if((int)p.row().read() == row && (int)p.col(),read() == col)
    //             return p;
    //     }
    //     return {0};
    // }
};


#ifdef SCRIPT_DLL
static DWORD* data_pos;

inline INJECTOR& INJECTOR::event1(const COMPARE& cond, int delay, const INJECTOR& Asm) {
    COMPARE comp = COMPARE(data_pos, EQUAL, -1) && cond;
    add(comp.if_jmp(INJECTOR()
                        .mov(EAX, p_myclock)
                        .add(EAX, delay)
                        .mov(data_pos, EAX)));
    mov(EAX, data_pos);
    cmp(EAX, p_myclock);
    if_jmp(jne, Asm);
    data_pos++;
    return *this;
}
inline INJECTOR& INJECTOR::event2(const COMPARE& cond, int delay, const INJECTOR& Asm,
                                  const COMPARE& cond2 = COMPARE()) {
    COMPARE comp = COMPARE(data_pos, NEQUAL, -2) && cond;
    add(comp.if_jmp(INJECTOR()
                        .mov(EAX, p_myclock)
                        .add(EAX, delay)
                        .mov(data_pos, EAX)));
    mov(EAX, data_pos);
    cmp(EAX, p_myclock);
    if_jmp(jne, cond2.if_jmp(INJECTOR(Asm)
                                 .mov(EAX, -2)
                                 .mov(data_pos, EAX)));
    data_pos++;
    return *this;
}

#define $ INJECTOR()

#endif