#define SCRIPT_DLL
#include "asm.h"

void Script(INJECTOR&);
extern "C" bool CallScript(INJECTOR&, int, const DLLARGS&, DLLRET&);
extern "C" void Result();

static GAMEPTR game;
static DWORD* cardTime;
static int(*Zombie)(const std::string&);
static int(*Plant)(const std::string&);

bool CallScript(INJECTOR& Asm, int ver, const DLLARGS& dllargs, DLLRET& dllret) {
    if(ver > DLLVERSION) {
        MessageBox(nullptr, "DLL�汾���ͣ��������°汾�Ĵ����ļ���", "����ʧ��", MB_ICONERROR);
        return false;
    }
    if(ver < DLLVERSION) {
        MessageBox(nullptr, "IZTools�汾���ͣ��������°汾��exe�ļ���", "����ʧ��", MB_ICONERROR);
        return false;
    }
    hGameProcess = dllargs.hProc;
    Zombie = dllargs.pfZombie;
    Plant = dllargs.pfPlant;
    cardTime = dllargs.cardTime;
    data_pos = p_eventflag;
    game.init();
    Script(Asm);
    game.data.zero();
    dllret.data_pos = data_pos;
    return true;
}