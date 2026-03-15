#pragma once
#include <windows.h>
#include <cstdint>
#include <iostream>
#include <tlhelp32.h>
#include <fstream>

namespace offest {
    
    constexpr uintptr_t dwEntityList = 0x24AE268;
    constexpr uintptr_t dwLocalPlayerController = 0x22F3178;
    constexpr uintptr_t dwLocalPlayerPawn = 0x2068B60;
    constexpr uintptr_t dwViewAngles = 0x2319648;
    constexpr uintptr_t m_hPlayerPawn = 0x90C;


    constexpr uintptr_t m_aimPunchAngle = 0x16CC;
    constexpr uintptr_t m_iShotsFired = 0x270C;

    constexpr uintptr_t m_iIDEntIndex = 0x3EAC;

    constexpr uintptr_t m_iTeamNum = 0x3F3;
    constexpr uintptr_t m_iHealth = 0x354;
}
extern HANDLE process;

namespace buff {
    extern uintptr_t clientdllbase;
    extern uintptr_t localcontroler;
    extern uintptr_t localPawn;
    extern uintptr_t EntityList;
    extern uintptr_t Controller;
    extern uintptr_t chunk0;
}

namespace trigger {
    extern int scope;
    extern uintptr_t scopecontroller;
    extern uintptr_t scopechunkbase;
    extern uint8_t Team;
    extern uint8_t EnemyTeam;
    extern bool bot;
}

namespace rts {
    extern int shotFired;
	extern bool recoil;
}

template <typename T>
inline T Read(uintptr_t address) {
    T value;
    ReadProcessMemory(process, (LPCVOID)address, &value, sizeof(T), NULL);
    return value;
}

template <typename T>
inline void Write(uintptr_t address, T value) {
    WriteProcessMemory(process, (LPVOID)address, &value, sizeof(T), NULL);
}
