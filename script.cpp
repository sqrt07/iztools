// g++ --share script.cpp -o script.dll -std=c++17

#include "script.h"

// Script函数在注入代码时执行一次
// Asm中的代码将在每帧种植植物和放置卡片后、判定结果前执行
void Script(INJECTOR& Asm) {
    // 【示例1】dstss 小鬼卡大喷相位
    // Asm.event1(game.plants[3].hp() < 200
    //         && game.plants[0].shootCountdown() == 10,
    //            $.use_card(3, 6, Zombie("xg"))); // 放卡片

    // 【示例2】tl1bw 撑杆卡冰豆相位
    // Asm.event2(game.plants[4].shootCountdown() == 2,
    //            $.use_card(3, 6, Zombie("cg")),
    //            152 - 34,
    //            game.plants[4].shootCountdown() < 2
    //         && game.zombies[0].haveHead() == 0
    // );

    // 【示例3】pddcp 撑杆卡后大喷相位
    // Asm.event2(game.plants[1].shootCountdown() == 1,
    //            $.use_card(3, 6, Zombie("cg")),
    //            150 - 49,
    //            game.plants[1].shootCountdown() < 2
    //         && game.zombies[0].state() == 1
    // );

    // 【示例4】bpslp 矿杆前后配合
    Asm.event1(game.plants[0].hp() < 250, // 220最佳
               $.use_card(3, 6, Zombie("cg"))
    );
    Asm.event1(game.zombies.cnt() == 2 && game.zombies[1].slowCountdown() != 0,
               $.lose().inc(game.mydata[0])
    ); // 记录撑杆被减速的次数

    // 【示例5】.1t.. 路障垫鬼  // 仅用于测试浮点数比较
    // Asm.event1(game.zombies[0].x() <= 320,
    //            $.use_card(3, 6, Zombie("xg"))
    // );
}

// Result函数在测试结束时执行一次
void Result() {
    game.mydata.show();
}