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
        MessageBox(nullptr, "DLL版本过低，请下载新版本的代码文件。", "加载失败", MB_ICONERROR);
        return false;
    }
    if(ver < DLLVERSION) {
        MessageBox(nullptr, "IZTools版本过低，请下载新版本的exe文件。", "加载失败", MB_ICONERROR);
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