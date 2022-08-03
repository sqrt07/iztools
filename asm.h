#include <windows.h>
#include <map>
#include <string>
#include <vector>
#include <set>
#include <sstream>
#include <fstream>
#include "memory.h"

static_assert(sizeof(void*) == 4, "请使用32位编译");

typedef int (*pfGETINT)(const std::string&);

enum REG { EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI };
enum FPU { ST0, ST1, ST2, ST3, ST4, ST5, ST6, ST7 };
enum CONDJMP : BYTE { jb  = 2, NBELOW = 2,
                      jnb = 3,  BELOW = 3,
                      je  = 4, NEQUAL = 4,
                      jne = 5,  EQUAL = 5,
                      jna = 6,  ABOVE = 6,
                      ja  = 7, NABOVE = 7 };
// 寄存器 + 偏移地址
template<typename T = DWORD>
struct PREG {
    REG r;
    int off;
    PREG(REG r, int offset = 0) : r(r), off(offset) {}
    PREG operator+(int offset) const {
        return {r, off + offset};
    }
};
inline static const PREG PEAX(EAX), PEBX(EBX), PECX(ECX), PEDX(EDX),
                         PESP(ESP), PEBP(EBP), PESI(ESI), PEDI(EDI);

inline DWORD* const p_eventflag = (DWORD*)0x700100;
inline DWORD* const p_myclock = (DWORD*)0x700010;
inline DWORD* const p_mylog = (DWORD*)0x701000;

class COMPARE;
#define $ INJECTOR()
#define $if $.if_
#define $mov $.mov
#define True COMPARE()
#define zb game.zombies
#define pl game.plants
#define bl game.bullets
class INJECTOR {
    std::vector<BYTE> code;

    static bool is_byte(long long x) {
        return x >= -128ll && x <= 127ll;
    }
   public:
    int len() const { return code.size(); }
    void clear() { code.clear(); }
    INJECTOR& add(const INJECTOR& Asm) {
        code.insert(code.end(), Asm.code.begin(), Asm.code.end());
        return *this;
    }
    INJECTOR& add_byte(BYTE c) {
        code.push_back(c);
        return *this;
    }
    INJECTOR& add_word(WORD c) { return add_byte(c).add_byte(c >> 8); }
    INJECTOR& add_dword(DWORD c) { return add_word(c).add_word(c >> 16); }
    INJECTOR& add_ptr(void* p) { return add_dword((DWORD)p); }
    /* 基本指令 */
    INJECTOR& nop() { return add_byte(0x90); }
    INJECTOR& ret() { return add_byte(0xc3); }
    INJECTOR& push(DWORD c) {  // push c
        return add_byte(0x68).add_dword(c);
    }
    INJECTOR& push(REG r) {  // push r
        return add_byte(0x50 + r);
    }
    INJECTOR& pop(REG r) { // pop r
        return add_byte(0x58 + r);
    }
    INJECTOR& call(DWORD c) {  // mov edi, c; call edi
        return mov(EDI, c).add_word(0xd7ff);
    }
    INJECTOR& mov(REG r, REG r2) {  // mov r, r2
        return add_byte(0x8b).add_byte(0xc0 + r * 8 + r2);
    }
    INJECTOR& mov(REG r, DWORD c) {  // mov r, c
        return add_byte(0xb8 + r).add_dword(c);
    }
    INJECTOR& mov(REG r, DWORD* p) {  // mov r, dword ptr[p]
        if(r == EAX) add_byte(0xa1);
        else add_byte(0x8b).add_byte(0x05 + r * 8);
        return add_ptr(p);
    }
    INJECTOR& mov(DWORD* p, REG r) {  // mov dword ptr[p], r
        if(r == EAX) add_byte(0xa3);
        else  add_byte(0x89).add_byte(0x05 + r * 8);
        return add_ptr(p);
    }
    INJECTOR& mov(DWORD* p, DWORD c) {  // mov dword ptr[p], c
        return add_word(0x05c7).add_ptr(p).add_dword(c);
    }
    INJECTOR& mov(WORD* p, WORD c) {  // mov word ptr[p], c
        return add_byte(0x66).add_word(0x05c7).add_ptr(p).add_word(c);
    }
    INJECTOR& mov(BYTE* p, BYTE c) {  // mov byte ptr[p], c
        return add_word(0x05c6).add_ptr(p).add_byte(c);
    }
    INJECTOR& mov(PREG<DWORD> pr, REG r) {  // mov dword ptr[pr + offset], r
        add_byte(0x89);
        if(pr.off == 0) return add_byte(r * 8 + pr.r);
        else if(is_byte(pr.off))
            return add_byte(0x40 + r * 8 + pr.r).add_byte(pr.off);
        else return add_byte(0x80 + r * 8 + pr.r).add_dword(pr.off);
    }
    INJECTOR& mov(REG r, PREG<DWORD> pr) { // mov r, dword ptr[pr + offset]
        add_byte(0x8b);
        if(pr.off == 0) return add_byte(r * 8 + pr.r);
        else if(is_byte(pr.off))
            return add_byte(0x40 + r * 8 + pr.r).add_byte(pr.off);
        else return add_byte(0x80 + r * 8 + pr.r).add_dword(pr.off);
    }
    INJECTOR& add(REG r, REG r2) {  // add r, r2
        return add_byte(0x01).add_byte(0xc0 + r2 * 8 + r);
    }
    INJECTOR& add(REG r, DWORD c) {  // add r, c
        if(is_byte(c)) return add_byte(0x83).add_byte(0xc0 + r).add_byte(c);
        if(r == EAX) add_byte(0x05);
        else add_byte(0x81).add_byte(0xc0 + r);
        return add_dword(c);
    }
    INJECTOR& add(DWORD* p, DWORD c) {  // add dword ptr[p], c
        if(is_byte(c)) return add_word(0x0583).add_ptr(p).add_byte(c);
        else return add_word(0x0581).add_ptr(p).add_dword(c);
    }
    INJECTOR& sub(REG r, DWORD c) {  // sub r, c
        if(is_byte(c)) return add_byte(0x83).add_byte(0xe8 + r).add_byte(c);
        if(r == EAX) add_byte(0x2d);
        else add_byte(0x81).add_byte(0xe8 + r);
        return add_dword(c);
    }
    INJECTOR& inc(DWORD* p) {  // inc dword ptr[p]
        return add_word(0x05ff).add_ptr(p);
    }
    INJECTOR& dec(DWORD* p) {  // dec dword ptr[p]
        return add_word(0x0dff).add_ptr(p);
    }
    INJECTOR& div(REG r) {  // div r
        return add_byte(0xf7).add_byte(0xf0 + r);
    }
    INJECTOR& cmp(REG r, DWORD c) {  // cmp r, c
        if(r == EAX) add_byte(0x3d);
        else add_byte(0x81).add_byte(0xf8 + r);
        return add_dword(c);
    }
    INJECTOR& cmp(REG r, DWORD* p) {  // cmp r, dword ptr[p]
        return add_byte(0x3b).add_byte(0x05 + r * 8).add_ptr(p);
    }
    INJECTOR& cmp(DWORD* p, DWORD c) {  // cmp dword ptr[p], c
        return add_word(0x3d81).add_ptr(p).add_dword(c);
    }
    INJECTOR& cmp(WORD* p, WORD c) {  // cmp word ptr[p], c
        return add_byte(0x66).add_word(0x3d81).add_ptr(p).add_word(c);
    }
    INJECTOR& cmp(BYTE* p, BYTE c) {  // cmp byte ptr[p], c
        return add_word(0x3d80).add_ptr(p).add_byte(c);
    }
    INJECTOR& repe() { return add_byte(0xf3); }
    INJECTOR& movsb() { return add_byte(0xa4); }
    INJECTOR& movsd() { return add_byte(0xa5); }
    INJECTOR& stosd() { return add_byte(0xab); }
    /* 浮点数 */
    INJECTOR& fld(float* p) {
        return add_word(0x05d9).add_ptr(p);
    }
    INJECTOR& fild(int* p) {
        return add_word(0x05db).add_ptr(p);
    }
    INJECTOR& fst(float* p) {
        return add_word(0x15d9).add_ptr(p);
    }
    INJECTOR& fstp(float* p) {
        return add_word(0x1dd9).add_ptr(p);
    }
    INJECTOR& fld_ESP() {  // fld [ESP]
        return add_byte(0xd9).add_word(0x2404);
    }
    INJECTOR& fstp_ESP() {  // fstp [ESP]
        return add_byte(0xd9).add_word(0x241c);
    }
    INJECTOR& fcom(float* p) {
        return add_word(0x15d8).add_ptr(p);
    }
    INJECTOR& fcomp(float* p) {
        return add_word(0x1dd8).add_ptr(p);
    }
    INJECTOR& fcomi(FPU ST) {
        return add_byte(0xdb).add_byte(0xf0 + ST);
    }
    INJECTOR& fcomip(FPU ST) {
        return add_byte(0xdf).add_byte(0xf0 + ST);
    }
    /* 控制 */
    INJECTOR& quit() {
        return mov(EAX, (DWORD*)0x700004)
              .add(EAX, 1)
              .mov((DWORD*)0x6ffff4, EAX);
    }
    INJECTOR& win() {
        return mov((BYTE*)0x700002, 2);
    }
    INJECTOR& lose() {
        return mov((BYTE*)0x700002, 1);
    }
    INJECTOR& log_clear() {
        return mov(p_mylog - 1, (DWORD)p_mylog);
    }
    INJECTOR& log(REG r) {
        return push(EDI)
              .mov(EDI, p_mylog - 1)
              .mov(PEDI, r)
              .add(p_mylog - 1, 4)
              .pop(EDI);
    }
    INJECTOR log(DWORD* p) {
        return push(EAX)
              .mov(EAX, p)
              .log(EAX)
              .pop(EAX);
    }
    INJECTOR log(void* p) {
        return push(EAX)
              .mov(EAX, (DWORD*)p)
              .log(EAX)
              .pop(EAX);
    }
    INJECTOR& cond_jmp(CONDJMP code, DWORD addr) {
        if(is_byte(addr))
            return add_byte(0x70 + code).add_byte(addr);
        else
            return add_byte(0x0f).add_byte(0x80 + code).add_dword(addr);
    }
    INJECTOR& jmp(DWORD addr) {
        if(is_byte(addr))
            return add_byte(0xeb).add_byte(addr);
        else
            return add_byte(0xe9).add_dword(addr);
    }
    INJECTOR& if_jmp(CONDJMP code, const INJECTOR& Asm) {
        return cond_jmp(code, Asm.len()).add(Asm);
    }
    // if语句（满足条件则执行）
    INJECTOR& if_(const COMPARE& cond, const INJECTOR& Asm);
    // if-else语句（满足条件则执行Asm，否则执行Asm2）
    INJECTOR& if_(const COMPARE& cond, const INJECTOR& Asm, const INJECTOR& Asm2);

    // 添加执行无数次的事件
    // cond:  触发条件（用&&连接要求同时满足的多个条件）
    // Asm:   执行的代码
    // delay: 触发后延迟的时间（延迟时不再次触发）
    // 例：event0(True, xxx, 1)创建了隔帧执行的事件
    INJECTOR& event0(const COMPARE& cond, const INJECTOR& Asm, DWORD delay = 0);

    // 添加只执行一次的事件（第一次触发）
    // cond:  触发条件（用&&连接要求同时满足的多个条件）
    // Asm:   执行的代码
    // delay: 触发后延迟的时间
    INJECTOR& event1(const COMPARE& cond, const INJECTOR& Asm, DWORD delay = 0);

    // 添加只执行一次的事件（最后一次触发：等待过程中如果再次触发条件，重新delay）
    // cond:  触发条件（用&&连接要求同时满足的多个条件）
    // delay: 触发后延迟的时间
    // Asm:   执行的代码
    // cond2: 二次判断的条件（可省略）：执行代码时检查是否满足条件，若不满足则取消执行，重新等待
    INJECTOR& event2(const COMPARE& cond, const INJECTOR& Asm, DWORD delay,
                     const COMPARE& cond2);

    // row和col从1开始；type使用Zombie("xx")
    INJECTOR& use_card(int row, int col, int type) {
        return push(col).push(row).push(type).call(0x6510b3);
    }
    // 显示白字
    INJECTOR& show_text(const std::string& text, int time) {
        push(EAX).push(EBX).push(ECX).push(EDX).push(ESI).push(EDI);

        std::string s = text;
        s.push_back('\0');
        while(s.length() % 4) s.push_back('\0');
        int n = s.length();
        for(int i = n - 4; i >= 0; i -= 4)
            push(*(DWORD*)(s.c_str() + i));
        mov(EAX, ESP); // 把文本存到栈上

        sub(ESP, 0x100);
        push(EAX); // Arg: 文本地址
        mov(ECX, ESP).add(ECX, 0x50).call(0x404450);
        mov(ESI, (DWORD*)0x6a9ec0)
        .mov(ESI, PESI + 0x768)
        .mov(ESI, PESI + 0x140);
        mov(ECX, 6);
        mov(EDX, ESP).add(EDX, 0x4c);
        call(0x459010);
        mov(EAX, time).mov(PESI + 0x88, EAX);

        add(ESP, n + 0x100);
        pop(EDI).pop(ESI).pop(EDX).pop(ECX).pop(EBX).pop(EAX);
        return *this;
    }
    // row和col从1开始；type使用Plant("xx")
    INJECTOR& new_plant(int row, int col, int type) {
        push(EAX).push(EBX).push(ECX).push(EDX).push(ESI).push(EDI);

        mov(EDI, (DWORD*)0x6a9ec0);
        mov(EDI, PEDI + 0x768);
        mov(EDI, PEDI + 0x160);
        _new_plant(row, col, type);

        pop(EDI).pop(ESI).pop(EDX).pop(ECX).pop(EBX).pop(EAX);
        return *this;
    }

    INJECTOR& _new_plant(int row, int col, int type) {
        push(col - 1);
        push(type);
        mov(EBX, row - 1);
        mov(EAX, 0x42a660).add_word(0xd0ff); // call 0x42a660
        return *this;
    }

    #ifndef SCRIPT_DLL
    /*调整行*/
    void put_plant(int row, int col, int type);
    void del_plant(int i);
    void del_zombie(int i);
    void del_bullet(int i);
    void clear_plants();
    void clear_zombies();
    void clear_bullets();

    /*内部使用*/
    void write(void* addr = (void*)0x651090);
    void prepareForDLL() {
        code.reserve(10000);
    }
    #endif
};

class COMPARE {
    friend class INJECTOR;
    std::vector<std::pair<INJECTOR, CONDJMP>> vec;

   public:
    // ptr指向的数据与c比较
    COMPARE(DWORD* ptr, CONDJMP cond, DWORD c) {
        vec.push_back({$.cmp(ptr, c), cond});
    }
    COMPARE(WORD* ptr, CONDJMP cond, WORD c) {
        vec.push_back({$.cmp(ptr, c), cond});
    }
    COMPARE(BYTE* ptr, CONDJMP cond, BYTE c) {
        vec.push_back({$.cmp(ptr, c), cond});
    }
    COMPARE(float* ptr, CONDJMP cond, float c) {
        vec.push_back({$.push(*(DWORD*)&c)
                        .fld_ESP().fld(ptr)
                        .fcomip(ST1).fstp_ESP()
                        .pop(EAX), cond});
    }

    // ptr指向的数据与ptr2指向的数据比较
    COMPARE(DWORD* ptr, CONDJMP cond, DWORD* ptr2) {
        vec.push_back({$.mov(EAX, ptr).cmp(EAX, ptr2), cond});
    }

    // 永远为真
    COMPARE() {}

    // 要求两个条件同时满足
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


class GAMEPTR {
   public:
    template<typename T>
    class MEMORY {
        T* ptr;
       public:
        MEMORY(T* ptr = nullptr) : ptr(ptr) {}
        operator T*() const { return ptr; }
        T read() const { return read_memory(ptr); }
        void write(T x) const { return write_memory(x, ptr); }
        COMPARE operator==(MEMORY<T> p2) const { return {ptr,  EQUAL, p2.ptr}; }
        COMPARE operator!=(MEMORY<T> p2) const { return {ptr, NEQUAL, p2.ptr}; }
        COMPARE operator< (MEMORY<T> p2) const { return {ptr,  BELOW, p2.ptr}; }
        COMPARE operator>=(MEMORY<T> p2) const { return {ptr, NBELOW, p2.ptr}; }
        COMPARE operator> (MEMORY<T> p2) const { return {ptr,  ABOVE, p2.ptr}; }
        COMPARE operator<=(MEMORY<T> p2) const { return {ptr, NABOVE, p2.ptr}; }
        COMPARE operator==(T x) const { return {ptr,  EQUAL, x}; }
        COMPARE operator!=(T x) const { return {ptr, NEQUAL, x}; }
        COMPARE operator< (T x) const { return {ptr,  BELOW, x}; }
        COMPARE operator>=(T x) const { return {ptr, NBELOW, x}; }
        COMPARE operator> (T x) const { return {ptr,  ABOVE, x}; }
        COMPARE operator<=(T x) const { return {ptr, NABOVE, x}; }
    };
    template<typename T>
    class POINTER {
        MEMORY<T> ptr;
       public:
        POINTER(T* ptr = 0) : ptr(ptr) {}
        T* p() const { return ptr; }
        MEMORY<T> operator*() const { return ptr; }
        MEMORY<T>* operator->() { return &ptr; }
        PREG<T> operator[](PREG<T> pr) const { return pr + (int)p(); }
        MEMORY<T> operator[](int x) const { return p() + x; }
        POINTER<T> operator+(int x) const { return p() + x; }
        POINTER<T> operator-(int x) const { return p() - x; }
        int operator-(POINTER<T> p2) const { return p() - p2.p(); }
    };
    template<typename T>
    class AUTOPOINTER : public POINTER<T> {
        using POINTER<T>::POINTER;
        std::set<int> used;
       public:
        MEMORY<T> operator[](int x) {
            used.insert(x);
            return this->p() + x;
        }
        void zero() const {
            for(int x : used)
                (*this + x)->write(0);
            write_memory((DWORD)p_mylog, p_mylog - 1);
        }
        void show() const {
            std::ostringstream sout;
            for(int x : used)
                sout << "[" << x << "] = " << (*this + x)->read() << '\n';
            MessageBox(nullptr, sout.str().c_str(), "data", MB_ICONINFORMATION);
        }
        void log(const char* filename = "log.txt", bool bfloat = false, int prec = 6, int nl = 1) const {
            std::ios::sync_with_stdio(false);
            std::ofstream fout(filename);
            if(bfloat) fout.precision(prec);
            fout.setf(std::ios::left);
            DWORD* pend = (DWORD*)read_memory(p_mylog - 1);
            int cnt = 0;
            for(DWORD* p = p_mylog; p < pend; ++p){
                fout.width(30);
                if(!bfloat) fout << read_memory(p);
                else fout << read_memory((float*)p);
                if(++cnt == nl) {
                    cnt = 0;
                    fout << '\n';
                }
            }
            fout.close();
        }
    };
    class ITEMBASE {
        int idx, ptr;
       public:
        ITEMBASE() = default;
        ITEMBASE(int idx, int ptr) : idx(idx), ptr(ptr) {}
        int p() const { return ptr; }
        template<typename T = DWORD>
        MEMORY<T> get(int offset) const { return (T*)(ptr + offset); }
        MEMORY<DWORD> operator[](int offset) const { return get(offset); }
    };
    template<typename ITEM, int size>
    class ITEMSBASE {
        int ptr, pitem;
       public:
        ITEMSBASE(int ptr = 0, int pitem = 0) : ptr(ptr), pitem(pitem) {}
        int p() const { return ptr; }
        MEMORY<DWORD> cnt_max() const { return (DWORD*)ptr + 1; }
        MEMORY<DWORD> next() const { return (DWORD*)ptr + 3; }
        MEMORY<DWORD> cnt() const { return (DWORD*)ptr + 4; }
        ITEM operator[](int idx) const { return {idx, pitem + idx * size}; }
    };
    struct PLANT : ITEMBASE {
        using ITEMBASE::ITEMBASE;
        // 以下函数命名与CVP保持一致
        MEMORY<DWORD> col() const { return get(0x28); } // 从0开始
        MEMORY<DWORD> state() const { return get(0x3c); }
        MEMORY<DWORD> hp() const { return get(0x40); }
        MEMORY<DWORD> propCountdown() const { return get(0x54); }
        MEMORY<DWORD> attackCountdown() const { return get(0x58); }
        MEMORY<DWORD> row() const { return get(0x88); } // 从0开始
        MEMORY<DWORD> shootCountdown() const { return get(0x90); }
        MEMORY<BYTE> isVanished() const { return get<BYTE>(0x141); }
    };
    struct ZOMBIE : ITEMBASE {
        using ITEMBASE::ITEMBASE;
        // 以下函数命名与CVP保持一致
        MEMORY<DWORD> state() const { return get(0x28); }
        MEMORY<float> x() const { return get<float>(0x2c); }
        MEMORY<float> y() const { return get<float>(0x30); }
        MEMORY<float> v() const { return get<float>(0x34); }
        MEMORY<BYTE> eating() const { return get<BYTE>(0x51); }
        MEMORY<DWORD> hp() const { return get(0xc8); }
        MEMORY<DWORD> helmetHp() const { return get(0xd0); }
        MEMORY<DWORD> shieldHp() const { return get(0xdc); }
        MEMORY<DWORD> slowCountdown() const { return get(0xac); }
        MEMORY<BYTE> haveHead() const { return get<BYTE>(0xba); }
        MEMORY<WORD> rank() { return get<WORD>(0x15a); }
        MEMORY<WORD> next() { return get<WORD>(0x158); }
    };
    struct BULLET : ITEMBASE {
        using ITEMBASE::ITEMBASE;
        // 以下函数命名与CVP保持一致
        // MEMORY<DWORD> col() const { return get(0x28); } // 从0开始
        // MEMORY<DWORD> state() const { return get(0x3c); }
        // MEMORY<DWORD> hp() const { return get(0x40); }
        // MEMORY<DWORD> propCountdown() const { return get(0x54); }
        // MEMORY<DWORD> attackCountdown() const { return get(0x58); }
        // MEMORY<DWORD> row() const { return get(0x88); } // 从0开始
        // MEMORY<DWORD> shootCountdown() const { return get(0x90); }
        // MEMORY<BYTE> isVanished() const { return get<BYTE>(0x141); }
    };
    int base, obj, plant, zombie, bullet, prog;
    MEMORY<DWORD> clock, mjclock, myclock;
    ITEMSBASE<PLANT, 0x14c> plants;
    ITEMSBASE<ZOMBIE, 0x15c> zombies;
    ITEMSBASE<PLANT, 0x94> bullets;
    AUTOPOINTER<DWORD> data;
    DWORD cardTime[20];
    void init() {
        base = read_memory<int>(0x6a9ec0);
        obj = read_memory<int>(base + 0x768);
        zombie = read_memory<int>(obj + 0x90);
        plant = read_memory<int>(obj + 0xac);
        bullet = read_memory<int>(obj + 0xc8);
        prog = read_memory<int>(obj + 0x160);
        clock = (DWORD*)(obj + 0x5568);
        mjclock = (DWORD*)(base + 0x838);
        myclock = p_myclock;
        plants = {obj + 0xac, plant};
        zombies = {obj + 0x90, zombie};
        bullets = {obj + 0xc8, bullet};
        data = {(DWORD*)0x700900};
    }
    // 判断编号为idx的僵尸是否已经放下
    COMPARE placed(int idx) const {
        return myclock >= cardTime[idx];
    }
};


#ifndef SCRIPT_DLL
#undef $
#undef $if
#undef mov
#undef True
#undef zb
#undef pl
/* 其他 */
enum class TEXTTYPE : BYTE { NONE, START, SAVE, PLAY, END, STOP };

#else

static DWORD* data_pos;

inline INJECTOR& INJECTOR::if_(const COMPARE& cond, const INJECTOR& Asm) {
    return add(cond.if_jmp(Asm));
}
inline INJECTOR& INJECTOR::if_(const COMPARE& cond, const INJECTOR& Asm, const INJECTOR& Asm2) {
    return add(cond.if_jmp(INJECTOR(Asm).jmp(Asm2.len()))).add(Asm2);
}
inline INJECTOR& INJECTOR::event0(const COMPARE& cond, const INJECTOR& Asm, DWORD delay) {
    COMPARE comp = COMPARE(data_pos, EQUAL, -1) && cond;
    add(comp.if_jmp($.mov(EAX, p_myclock)
                     .add(EAX, delay)
                     .mov(data_pos, EAX)));
    mov(EAX, data_pos);
    cmp(EAX, p_myclock);
    if_jmp(jne, INJECTOR(Asm).mov(EAX, -1).mov(data_pos, EAX));
    data_pos++;
    return *this;
}
inline INJECTOR& INJECTOR::event1(const COMPARE& cond, const INJECTOR& Asm, DWORD delay) {
    COMPARE comp = COMPARE(data_pos, EQUAL, -1) && cond;
    add(comp.if_jmp($.mov(EAX, p_myclock)
                     .add(EAX, delay)
                     .mov(data_pos, EAX)));
    mov(EAX, data_pos);
    cmp(EAX, p_myclock);
    if_jmp(jne, Asm);
    data_pos++;
    return *this;
}
inline INJECTOR& INJECTOR::event2(const COMPARE& cond, const INJECTOR& Asm, DWORD delay,
                                  const COMPARE& cond2 = COMPARE()) {
    COMPARE comp = COMPARE(data_pos, NEQUAL, -2) && cond;
    add(comp.if_jmp($.mov(EAX, p_myclock)
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

#endif

struct DLLARGS {
    HANDLE hProc;
    pfGETINT pfPlant, pfZombie;
    DWORD* cardTime;
};
struct DLLRET {
    DWORD* data_pos;
};
#define DLLVERSION 22'04'10'00