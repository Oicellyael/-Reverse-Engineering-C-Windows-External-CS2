#include <iostream>
#include <Windows.h>
#include <tlhelp32.h>
#include "help/help.h"
using namespace std;

namespace buff {
    uintptr_t clientdllbase;
    uintptr_t localcontroler = 0;
    uintptr_t localPawn = 0;
    uintptr_t EntityList = 0;
    uintptr_t Controller = 0;
    int pawnindex = 0;
    uintptr_t Pawn = 0;
    uintptr_t firstEntity = 0;
    uint32_t hpPawn = 0;
    uintptr_t chunk0 = 0;
}

int main() {
    HWND window = FindWindowA(NULL, "Counter-Strike 2");
    DWORD pid;
    GetWindowThreadProcessId(window, &pid);
    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    HANDLE snap2 = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    MODULEENTRY32 mod;
    mod.dwSize = sizeof(mod);
    Module32First(snap2, &mod);
    do {
        if (_wcsicmp(mod.szModule, L"client.dll") == 0) {
            buff::clientdllbase = reinterpret_cast<uintptr_t>(mod.modBaseAddr);
            break;
        }
    } while (Module32Next(snap2, &mod));
    CloseHandle(snap2);
    //ReadProcessMemory(process, (LPCVOID)(buff::clientdllbase + offest::dwLocalPlayerController), &buff::localcontroler, sizeof(buff::localcontroler), NULL);
    ReadProcessMemory(process, (LPCVOID)(buff::clientdllbase + offest::dwEntityList), &buff::EntityList, sizeof(buff::EntityList), NULL);
    cout << "entitymain: " << buff::EntityList << endl;
    while (!GetAsyncKeyState(VK_DELETE)) {
        system("cls");
        ReadProcessMemory(process, (LPCVOID)(buff::clientdllbase + offest::dwLocalPlayerController), &buff::localcontroler, sizeof(buff::localcontroler), NULL);
        uint8_t myTeam = 0;
        ReadProcessMemory(process, (LPCVOID)(buff::localcontroler + offest::m_iTeamNum), &myTeam, sizeof(myTeam), NULL);
        ReadProcessMemory(process, (LPCVOID)(buff::EntityList + 0x10), &buff::chunk0, sizeof(buff::chunk0), NULL);
        for (int i = 0; i < 64; i++) {
            ReadProcessMemory(process, (LPCVOID)(buff::chunk0 + i * 112), &buff::Controller, sizeof(buff::Controller), NULL);
            if (!buff::Controller) continue;
            if (buff::Controller == buff::localcontroler) continue;

            /*uint8_t enemyTeam = 0;
            ReadProcessMemory(process, (LPCVOID)(buff::Controller + 0x3F3), &enemyTeam, sizeof(enemyTeam), NULL);
            if (enemyTeam == myTeam || enemyTeam == 0) continue;*/

            // получаем pawn
            int pawnHandle = 0;
            ReadProcessMemory(process, (LPCVOID)(buff::Controller + offest::m_hPlayerPawn), &pawnHandle, sizeof(pawnHandle), NULL);
            if (!pawnHandle || pawnHandle == -1) continue;
            int pawnIndex = pawnHandle & 0x7FFF;

            uintptr_t pawnChunk = 0;
            ReadProcessMemory(process, (LPCVOID)(buff::EntityList + (pawnIndex >> 9) * 8 + 0x10), &pawnChunk, sizeof(pawnChunk), NULL);
            if (!pawnChunk) continue;

            uintptr_t pawn = 0;
            ReadProcessMemory(process, (LPCVOID)(pawnChunk + (pawnIndex & 0x1FF) * 112), &pawn, sizeof(pawn), NULL);
            if (!pawn) continue;

            int hp = 0;
            ReadProcessMemory(process, (LPCVOID)(pawn + offest::m_iHealth), &hp, sizeof(hp), NULL);
            if (hp <= 0 || hp > 100) continue;

            cout << "Enemy " << i << " HP: " << dec << hp << endl;
        }
        Sleep(500);
    }
}