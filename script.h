#define SCRIPT_DLL
#include "asm.h"

void Script(INJECTOR&);
extern "C" void CallScript(INJECTOR&, HANDLE, pfGETINT, pfGETINT);
extern "C" void Result();

static GAMEPTR gp;
static int(*Zombie)(const std::string&);
static int(*Plant)(const std::string&);

void CallScript(INJECTOR& Asm, HANDLE hProc, pfGETINT pfPlant, pfGETINT pfZombie) {
    hGameProcess = hProc;
    Zombie = pfZombie;
    Plant = pfPlant;
    data_pos = (DWORD*)0x700100;
    gp.init();
    Script(Asm);
}