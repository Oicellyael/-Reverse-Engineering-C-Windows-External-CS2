#include <iostream>
#include <Windows.h>
#include <tlhelp32.h>
#include "help/help.h"
#include <fstream>
#include <string>
using namespace std;
INPUT clicks[2] = {};
struct Vector2 {
    float x{}, y{};
};
Vector2 oldPunch = {};
void SaveConfig() {
    ofstream file("config.ini");
    file << "triggerbot=" << trigger::bot << endl;
    file << "rcs=" << rts::recoil << endl;
    file.close();
}
void LoadConfig() {
    ifstream file("config.ini");
    string line;
    while (getline(file, line)) {
        line.find("triggerbot=");
        if (line.find("triggerbot=") == 0) {
            trigger::bot = stoi(line.substr(11)); // stoi конвертит строку в int, потом в bool
        }
        if (line.find("rcs=") == 0) {
            rts::recoil = stoi(line.substr(4));
        }
    }
    file.close();
}

int main() {
    HWND window = FindWindowA(NULL, "Counter-Strike 2");
    DWORD pid;
    GetWindowThreadProcessId(window, &pid);
    process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
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
    clicks[0].type = clicks[1].type = INPUT_MOUSE;
    clicks[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    clicks[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    LoadConfig();
    buff::EntityList = Read<uintptr_t>(buff::clientdllbase + offest::dwEntityList);
    while (!GetAsyncKeyState(VK_DELETE)) {
        if (GetAsyncKeyState(VK_F5) & 1) SaveConfig();
        // Читаем контроллер и павн локального игрока
        buff::localcontroler = Read<uintptr_t>(buff::clientdllbase + offest::dwLocalPlayerController);
        buff::localPawn = Read<uintptr_t>(buff::clientdllbase + offest::dwLocalPlayerPawn);
        buff::chunk0 = Read<uintptr_t>(buff::EntityList + 0x10);
        if (GetAsyncKeyState(VK_NUMPAD1) & 1) trigger::bot = !trigger::bot;
        if (trigger::bot) {
                trigger::scope = Read<int>(buff::localPawn + offest::m_iIDEntIndex);
                if (trigger::scope != -1) {
                    trigger::scopechunkbase = Read<uintptr_t>(buff::EntityList + (trigger::scope >> 9) * 8 + 0x10);
                    trigger::scopecontroller = Read<uintptr_t>(trigger::scopechunkbase + (trigger::scope & 0x1FF) * 112);

                    trigger::Team = Read<uint8_t>(buff::localcontroler + offest::m_iTeamNum);
                    trigger::EnemyTeam = Read<uint8_t>(trigger::scopecontroller + offest::m_iTeamNum);

                    if (trigger::EnemyTeam != trigger::Team && trigger::EnemyTeam != 0) {
                        SendInput(1, &clicks[0], sizeof(INPUT));
                        Sleep(rand() % 100);
                        SendInput(1, &clicks[1], sizeof(INPUT));
                    }
                }
				Sleep(100);
        }
    

        if (GetAsyncKeyState(VK_NUMPAD2) & 1) rts::recoil = !rts::recoil; 
        if (rts::recoil) {
                rts::shotFired = Read<int>(buff::localPawn + offest::m_iShotsFired);
                if (rts::shotFired > 1) {
                    Vector2 punch = Read<Vector2>(buff::localPawn + offest::m_aimPunchAngle);
                    Vector2 viewAngles = Read<Vector2>(buff::clientdllbase + offest::dwViewAngles);
                    Vector2 newAngle = {
                        viewAngles.x + oldPunch.x - punch.x * 1.0f,
                        viewAngles.y + oldPunch.y - punch.y * 1.0f
                    };
                    oldPunch = punch;
                    Write(buff::clientdllbase + offest::dwViewAngles, newAngle);
                }
                else {
                    oldPunch = { 0, 0 };
                }
            
        }
    
        for (int i = 0; i < 64; i++) {
            buff::Controller = Read<uintptr_t>(buff::chunk0 + i * 112);
            if (!buff::Controller || buff::Controller == buff::localcontroler) continue;

            int pawnHandle = Read<int>(buff::Controller + offest::m_hPlayerPawn);
            if (!pawnHandle || pawnHandle == -1) continue;

            int pawnIndex = pawnHandle & 0x7FFF;
            uintptr_t pawnChunk = Read<uintptr_t>(buff::EntityList + (pawnIndex >> 9) * 8 + 0x10);
            if (!pawnChunk) continue;

            uintptr_t pawn = Read<uintptr_t>(pawnChunk + (pawnIndex & 0x1FF) * 112);
            if (!pawn) continue;

            uint8_t myTeam = Read<uint8_t>(buff::localcontroler + offest::m_iTeamNum);
            uint8_t enemyTeam = Read<uint8_t>(buff::Controller + offest::m_iTeamNum);

            if (enemyTeam != myTeam && enemyTeam != 0) {
                int hp = Read<int>(pawn + offest::m_iHealth);
                if (hp > 0 && hp <= 100) {
                 
                }
            }
        }
        Sleep(2);
    }
}
