#define SCRIPT_DLL
#include "asm.h"

void Script(INJECTOR&);
extern "C" DLLRET CallScript(INJECTOR&, HANDLE, pfGETINT, pfGETINT);
extern "C" void Result();

static GAMEPTR game;
static int(*Zombie)(const std::string&);
static int(*Plant)(const std::string&);

DLLRET CallScript(INJECTOR& Asm, HANDLE hProc, pfGETINT pfPlant, pfGETINT pfZombie) {
    hGameProcess = hProc;
    Zombie = pfZombie;
    Plant = pfPlant;
    data_pos = p_eventflag;
    game.init();
    Script(Asm);
    game.mydata.zero();
    return {data_pos};
}