#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <direct.h>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "asm.h"
#include "debug.h"
#include "resource.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK SetChildWndFont(HWND hWnd, LPARAM lParam);
extern HINSTANCE hInst;
extern HWND hWnd;
extern HMENU hMenu;
extern HFONT hFont;

extern HWND hGameWindow;

extern bool bRunning;

void EndTest(bool flag = false);
void InitTextMap();
int FindPlantInMap(std::string s);
int FindZombieInMap(std::string s);

extern std::map<std::string, int> m_p, m_z;
extern GAMEPTR gp;

struct Args {
    BYTE PlantType[5][5];
    BYTE ZombieType[20];
    DWORD ZombieTime[20];
    BYTE ZombieCol[20], ZombieRow[20];
    BYTE KeyCol[10], KeyRow[10];
    BYTE ZombieCnt, KeyCnt;
    DWORD Total;
    DWORD CardTime;
    DWORD Mjlock;
    BYTE NoMj;
};

void* AllocMemory(int size);
void FreeMemory(void*& p);

extern LPCSTR str_help, str_help5, str_char, str_about;

#define VERSION "v1.5.0"