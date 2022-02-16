// g++ --share script.cpp -o script.dll -std=c++17

#include "script.h"

// Script函数在注入代码时执行一次
// Asm中的代码将在每帧种植植物和放置卡片后、判定结果前执行
void Script(INJECTOR& Asm) {
    // 【示例1】dstss 小鬼卡大喷相位
    Asm.event1(COMPARE((DWORD*)(gp.get_plant(3) + 0x40), BELOW, 200) // 伞叶血量低于200
            && COMPARE((DWORD*)(gp.get_plant(0) + 0x90), EQUAL, 10), // 大喷攻击倒计时为9
               0,
               INJECTOR().use_card(3, 6, Zombie("xg"))); // 放卡片

    // 【示例2】tl1bw 撑杆卡冰豆相位
    // Asm.event2(COMPARE((DWORD*)(gp.get_plant(4) + 0x90), EQUAL, 2),
    //            152 - 34,
    //            INJECTOR().use_card(3, 6, m_z["cg"]),
    //            COMPARE((DWORD*)(gp.get_plant(4) + 0x90), BELOW, 2)
    //         && COMPARE((BYTE*)(gp.get_zombie(0) + 0xba), EQUAL, 0)
    // );

    // 【示例3】pddcp 撑杆卡后大喷相位
    // Asm.event2(COMPARE((DWORD*)(gp.get_plant(1) + 0x90), EQUAL, 1),
    //            150 - 49,
    //            INJECTOR().use_card(3, 6, m_z["cg"]),
    //            COMPARE((DWORD*)(gp.get_plant(1) + 0x90), BELOW, 2)
    //         && COMPARE((DWORD*)(gp.get_zombie(0) + 0x28), EQUAL, 1)
    // );
}

// Result函数在测试结束时执行一次
void Result() {}