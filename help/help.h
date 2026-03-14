#pragma once
#include <windows.h>
#include <iostream>
#include <limits>
#include <vector>
namespace offest {
    //offsets.hpp
    constexpr uintptr_t dwEntityList = 0x24AE268;
    constexpr uintptr_t dwLocalPlayerController = 0x22F3178;
    constexpr uintptr_t dwLocalPlayerPawn = 0x2068B60;
    constexpr uintptr_t dwViewMatrix = 0x230EF20;
    constexpr uintptr_t dwViewAngles = 0x2319648;

    // client_dll.hpp
    constexpr uintptr_t m_iTeamNum = 0x3F3;
    constexpr uintptr_t m_hPawn = 0x6C4;
    constexpr uintptr_t m_hPlayerPawn = 0x90C;
    constexpr uintptr_t m_iIDEntIndex = 0x3EAC;

    constexpr uintptr_t m_iHealth = 0x354;
    constexpr uintptr_t m_iPawnHealth = 0x918;
    constexpr uintptr_t m_lifeState = 0x35C;
    constexpr uintptr_t m_pGameSceneNode = 0x338;
    constexpr uintptr_t boneArray = 0x160 + 0x80;
}