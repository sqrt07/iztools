// g++ --share script.cpp -o script.dll -std=c++17

#include "script.h"

// Script������ע�����ʱִ��һ��
// Asm�еĴ��뽫��ÿ֡��ֲֲ��ͷ��ÿ�Ƭ���ж����ǰִ��
void Script(INJECTOR& Asm) {
    // ��ʾ��1��dstss С��������λ
    // Asm.event1(gp.plants[3].hp() < 200
    //         && gp.plants[0].shootCountdown() == 10,
    //            0, $.use_card(3, 6, Zombie("xg"))); // �ſ�Ƭ

    // ��ʾ��2��tl1bw �Ÿ˿�������λ
    // Asm.event2(gp.plants[4].shootCountdown() == 2,
    //            152 - 34, $.use_card(3, 6, Zombie("cg")),
    //            gp.plants[4].shootCountdown() < 2
    //         && gp.zombies[0].haveHead() == 0
    // );

    // ��ʾ��3��pddcp �Ÿ˿��������λ
    // Asm.event2(gp.plants[1].shootCountdown() == 1,
    //            150 - 49, $.use_card(3, 6, Zombie("cg")),
    //            gp.plants[1].shootCountdown() < 2
    //         && gp.zombies[0].state() == 1
    // );

    // ��ʾ��4��bpslp ���ǰ�����
    // Asm.event1(gp.plants[0].hp() < 220,
    //            0, $.use_card(3, 6, Zombie("cg"))
    // );
    // Asm.event1(gp.plants.cnt() == 2 && gp.zombies[1].slowCountdown() != 0,
    //            0, $.lose().quit()
    // ); // �Ÿ˱����پ��˳�
}

// Result�����ڲ��Խ���ʱִ��һ��
void Result() {}