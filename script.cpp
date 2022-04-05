// g++ --share script.cpp -o script.dll -std=c++17

#include "script.h"

// Script函数在注入代码时执行一次
// Asm中的代码将在每帧种植植物和放置卡片后、判定结果前执行
void Script(INJECTOR& Asm) {
    // 【示例1】dstss 小鬼卡大喷相位
    // Asm.event1(gp.plants[3].hp() < 200
    //         && gp.plants[0].shootCountdown() == 10,
    //            0, $.use_card(3, 6, Zombie("xg"))); // 放卡片

    // 【示例2】tl1bw 撑杆卡冰豆相位
    // Asm.event2(gp.plants[4].shootCountdown() == 2,
    //            152 - 34, $.use_card(3, 6, Zombie("cg")),
    //            gp.plants[4].shootCountdown() < 2
    //         && gp.zombies[0].haveHead() == 0
    // );

    // 【示例3】pddcp 撑杆卡后大喷相位
    // Asm.event2(gp.plants[1].shootCountdown() == 1,
    //            150 - 49, $.use_card(3, 6, Zombie("cg")),
    //            gp.plants[1].shootCountdown() < 2
    //         && gp.zombies[0].state() == 1
    // );

    // 【示例4】bpslp 矿杆前后配合
    // Asm.event1(gp.plants[0].hp() < 220,
    //            0, $.use_card(3, 6, Zombie("cg"))
    // );
    // Asm.event1(gp.plants.cnt() == 2 && gp.zombies[1].slowCountdown() != 0,
    //            0, $.lose().quit()
    // ); // 撑杆被减速就退出
}

// Result函数在测试结束时执行一次
void Result() {}