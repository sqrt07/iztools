// g++ --share script.cpp -o script.dll -std=c++17

#include "script.h"

// Script������ע�����ʱִ��һ��
// Asm�еĴ��뽫��ÿ֡��ֲֲ��ͷ��ÿ�Ƭ���ж����ǰִ��
void Script(INJECTOR& Asm) {
    // ��ʾ��1��dstss С��������λ
    // Asm.event1(game.plants[3].hp() < 200
    //         && game.plants[0].shootCountdown() == 10,
    //            $.use_card(3, 6, Zombie("xg"))); // �ſ�Ƭ

    // ��ʾ��2��tl1bw �Ÿ˿�������λ
    // Asm.event2(game.plants[4].shootCountdown() == 2,
    //            $.use_card(3, 6, Zombie("cg")),
    //            152 - 34,
    //            game.plants[4].shootCountdown() < 2
    //         && game.zombies[0].haveHead() == 0
    // );

    // ��ʾ��3��pddcp �Ÿ˿��������λ
    // Asm.event2(game.plants[1].shootCountdown() == 1,
    //            $.use_card(3, 6, Zombie("cg")),
    //            150 - 49,
    //            game.plants[1].shootCountdown() < 2
    //         && game.zombies[0].state() == 1
    // );

    // ��ʾ��4��bpslp ���ǰ�����
    Asm.event1(game.plants[0].hp() < 250, // 220���
               $.use_card(3, 6, Zombie("cg"))
    );
    Asm.event1(game.zombies.cnt() == 2 && game.zombies[1].slowCountdown() != 0,
               $.lose().inc(game.mydata[0])
    ); // ��¼�Ÿ˱����ٵĴ���

    // ��ʾ��5��.1t.. ·�ϵ��  // �����ڲ��Ը������Ƚ�
    // Asm.event1(game.zombies[0].x() <= 320,
    //            $.use_card(3, 6, Zombie("xg"))
    // );
}

// Result�����ڲ��Խ���ʱִ��һ��
void Result() {
    game.mydata.show();
}