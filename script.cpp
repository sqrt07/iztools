// g++ --share script.cpp -o script.dll -std=c++17

#include "script.h"

// Script������ע�����ʱִ��һ��
// Asm�еĴ��뽫��ÿ֡��ֲֲ��ͷ��ÿ�Ƭ���ж����ǰִ��
void Script(INJECTOR& Asm) {
    // ��ʾ��1��dstss С��������λ
    Asm.event1(COMPARE((DWORD*)(gp.get_plant(3) + 0x40), BELOW, 200) // ɡҶѪ������200
            && COMPARE((DWORD*)(gp.get_plant(0) + 0x90), EQUAL, 10), // ���繥������ʱΪ9
               0,
               INJECTOR().use_card(3, 6, Zombie("xg"))); // �ſ�Ƭ

    // ��ʾ��2��tl1bw �Ÿ˿�������λ
    // Asm.event2(COMPARE((DWORD*)(gp.get_plant(4) + 0x90), EQUAL, 2),
    //            152 - 34,
    //            INJECTOR().use_card(3, 6, m_z["cg"]),
    //            COMPARE((DWORD*)(gp.get_plant(4) + 0x90), BELOW, 2)
    //         && COMPARE((BYTE*)(gp.get_zombie(0) + 0xba), EQUAL, 0)
    // );

    // ��ʾ��3��pddcp �Ÿ˿��������λ
    // Asm.event2(COMPARE((DWORD*)(gp.get_plant(1) + 0x90), EQUAL, 1),
    //            150 - 49,
    //            INJECTOR().use_card(3, 6, m_z["cg"]),
    //            COMPARE((DWORD*)(gp.get_plant(1) + 0x90), BELOW, 2)
    //         && COMPARE((DWORD*)(gp.get_zombie(0) + 0x28), EQUAL, 1)
    // );
}

// Result�����ڲ��Խ���ʱִ��һ��
void Result() {}