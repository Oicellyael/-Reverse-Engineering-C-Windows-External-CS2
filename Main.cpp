#pragma comment(lib, "d3d11.lib")
#include "help/help.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <d3d11.h>
#include "imgui/imgui.h"
#include "imgui/imconfig.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

using namespace std;

INPUT clicks[2] = {};
Vector2 oldPunch = { 0, 0 };

void RunTriggerbot() {
    trigger::scope = Read<int>(buff::localPawn + offest::m_iIDEntIndex);
    if (trigger::scope != -1) {
        trigger::scopechunkbase = Read<uintptr_t>(buff::EntityList + (trigger::scope >> 9) * 8 + 0x10);
        trigger::scopecontroller = Read<uintptr_t>(trigger::scopechunkbase + (trigger::scope & 0x1FF) * 112);

        trigger::Team = Read<uint8_t>(buff::localcontroler + offest::m_iTeamNum);
        trigger::EnemyTeam = Read<uint8_t>(trigger::scopecontroller + offest::m_iTeamNum);

        if (trigger::EnemyTeam != trigger::Team && trigger::EnemyTeam != 0) {
           
            SendInput(1, &clicks[0], sizeof(INPUT));
            Sleep(rand() % 60); 
            SendInput(1, &clicks[1], sizeof(INPUT));
        }
    }
}

void RunRCS() {
    rts::shotFired = Read<int>(buff::localPawn + offest::m_iShotsFired);
    if (rts::shotFired > 1) {
        Vector2 punch = Read<Vector2>(buff::localPawn + offest::m_aimPunchAngle);
        Vector2 viewAngles = Read<Vector2>(buff::clientdllbase + offest::dwViewAngles);

        Vector2 newAngle = {
            viewAngles.x + oldPunch.x - punch.x * 1.0f, // Коэффициент 2.0 для CS
            viewAngles.y + oldPunch.y - punch.y * 1.0f
        };

        ClampAngles(newAngle);

        oldPunch = punch;
        Write<Vector2>(buff::clientdllbase + offest::dwViewAngles, newAngle);
    }
    else {
        oldPunch = { 0, 0 };
    }
}

DWORD WINAPI GlowThread(LPVOID) {
    while (true) {
        if (!buff::glow) {
            Sleep(100);
            continue;
        }
        if (buff::chunk0 && buff::localcontroler) {
            // Читаем твою команду один раз за проход цикла по всем игрокам
            uint8_t myTeam = Read<uint8_t>(buff::localcontroler + offest::m_iTeamNum);

            for (int i = 0; i < 64; i++) {
                uintptr_t controller = Read<uintptr_t>(buff::chunk0 + i * 112);
                if (!controller || controller == buff::localcontroler) continue;

                // Проверяем команду этого игрока
                uint8_t enemyTeam = Read<uint8_t>(controller + offest::m_iTeamNum);

                // Если это союзник или команда не определена (0) — пропускаем
                if (enemyTeam == myTeam || enemyTeam == 0) continue;

                int pawnHandle = Read<int>(controller + offest::m_hPlayerPawn);
                int pawnIndex = pawnHandle & 0x7FFF;
                uintptr_t pawnChunk = Read<uintptr_t>(buff::EntityList + (pawnIndex >> 9) * 8 + 0x10);
                uintptr_t pawn = Read<uintptr_t>(pawnChunk + (pawnIndex & 0x1FF) * 112);

                if (!pawn) continue;

                // Проверка на HP (чтобы не светить трупы)
                int health = Read<int>(pawn + offest::m_iHealth);
                if (health <= 0 || health > 100) continue;

                uintptr_t glowBase = pawn + offest::m_Glow;
                Write<int>(glowBase + 0x40, 0xFF0000FF); // Красный для врагов
                Write<int>(glowBase + 0x30, 1);
                Write<bool>(glowBase + offest::m_bGlowing, true);
            }
        }
		Sleep(2);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(hwnd, msg, wp, lp);
}

MSG msg;
DWORD WINAPI RenderThread(LPVOID) {
    while (true) {
        /* if (!buff::radar) {
             Sleep(100);
             continue;
         }*/
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Vector3 localOrigin = Read<Vector3>(buff::localPawn + offest::m_vOldOrigin);
        uint8_t myTeam = Read<uint8_t>(buff::localcontroler + offest::m_iTeamNum);
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        float clearColor[4] = { 0, 0, 0, 0 }; // прозрачный
        pContext->ClearRenderTargetView(pRenderTargetView, clearColor);

        ImDrawList* DrawList = ImGui::GetBackgroundDrawList();
        float scale = 0.1f;
        float centerX = 200, centerY = 200; // центр окна
        DrawList->AddRectFilled(ImVec2(0, 0), ImVec2(700, 700), ImColor(20, 20, 20, 200));

        for (int i = 0; i < 64; i++) {
            uintptr_t controller = Read<uintptr_t>(buff::chunk0 + i * 112);
            if (!controller || controller == buff::localcontroler) continue;

            // Проверяем команду этого игрока
            uint8_t enemyTeam = Read<uint8_t>(controller + offest::m_iTeamNum);

            // Если это союзник или команда не определена (0) — пропускаем
            if (enemyTeam == myTeam || enemyTeam == 0) continue;

            int pawnHandle = Read<int>(controller + offest::m_hPlayerPawn);
            int pawnIndex = pawnHandle & 0x7FFF;
            uintptr_t pawnChunk = Read<uintptr_t>(buff::EntityList + (pawnIndex >> 9) * 8 + 0x10);
            uintptr_t pawn = Read<uintptr_t>(pawnChunk + (pawnIndex & 0x1FF) * 112);

            if (!pawn) continue;
            // Проверка на HP (чтобы не светить трупы)
            int health = Read<int>(pawn + offest::m_iHealth);
            if (health <= 0 || health > 100) continue; {
                Vector3 origin = Read<Vector3>(pawn + offest::m_vOldOrigin);
                float scale = 0.1f;
                float centerX = 200, centerY = 200;
                float px = centerX + (origin.x - localOrigin.x) * scale;
                float py = centerY + (origin.y - localOrigin.y) * scale;
                DrawList->AddCircleFilled(ImVec2(px, py), 5, ImColor(255, 0, 0, 255));
            }
        }
        ImGui::Render();
        pContext->OMSetRenderTargets(1, &pRenderTargetView, NULL);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        swapchain->Present(1, 0);
    }
    
}

int main() {
    HWND window = FindWindowA(NULL, "Counter-Strike 2");
    DWORD pid;
    GetWindowThreadProcessId(window, &pid);
    process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    buff::clientdllbase = GetModuleBase(pid, L"client.dll");

    // Инициализация мышки
    clicks[0].type = clicks[1].type = INPUT_MOUSE;
    clicks[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    clicks[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

    LoadConfig();
    buff::EntityList = Read<uintptr_t>(buff::clientdllbase + offest::dwEntityList);
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);        // размер структуры
    wc.style = CS_HREDRAW | CS_VREDRAW;   // стиль
    wc.lpfnWndProc = WndProc;             // функция обработки сообщений
    wc.hInstance = GetModuleHandle(NULL); // инстанс
    wc.lpszClassName = L"RadarClass";     // имя класса
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        L"RadarClass",
        L"Radar",
        WS_POPUP,
        0, 0, 400, 400,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(hwnd, SW_SHOW);
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.Width = 600;
    sd.BufferDesc.Height = 600;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
        NULL, 0, D3D11_SDK_VERSION,
        &sd, &swapchain, &dev, NULL, &pContext
    );
    ID3D11Texture2D* pBackBuffer;
    swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    dev->CreateRenderTargetView(pBackBuffer, NULL, &pRenderTargetView);
    pBackBuffer->Release();

    ImGui::CreateContext();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(dev, pContext);
    CreateThread(nullptr, 0, GlowThread, nullptr, 0, nullptr);
    CreateThread(nullptr, 0, RenderThread, nullptr, 0, nullptr);
    cout << "Cheat Started. F5: Save Config, NUM1: Trigger, NUM2: RCS" << endl;

    while (!GetAsyncKeyState(VK_DELETE)) {
        MSG msg2;
        while (PeekMessage(&msg2, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg2);
            DispatchMessage(&msg2);
        }
        buff::localcontroler = Read<uintptr_t>(buff::clientdllbase + offest::dwLocalPlayerController);
        buff::localPawn = Read<uintptr_t>(buff::clientdllbase + offest::dwLocalPlayerPawn);
        buff::chunk0 = Read<uintptr_t>(buff::EntityList + 0x10);

        if (GetAsyncKeyState(VK_F5) & 1) { SaveConfig(); cout << "Config Saved!" << endl; }
        if (GetAsyncKeyState(VK_XBUTTON1) & 1) { trigger::bot = !trigger::bot; cout << "Trigger: " << trigger::bot << endl; }
        if (GetAsyncKeyState(VK_NUMPAD1) & 1) { rts::recoil = !rts::recoil; cout << "RCS: " << rts::recoil << endl; }
        if (GetAsyncKeyState(VK_NUMPAD2) & 1) { buff::glow = !buff::glow; cout << "Glow: " << (buff::glow ? "ON" : "OFF") << endl; }
        //if (GetAsyncKeyState(VK_NUMPAD0) & 1) { buff::radar = !buff::radar;}

        if (trigger::bot) RunTriggerbot();
        if (rts::recoil)  RunRCS();

        uint8_t myTeam = Read<uint8_t>(buff::localcontroler + offest::m_iTeamNum);

        for (int i = 0; i < 64; i++) {
            uintptr_t controller = Read<uintptr_t>(buff::chunk0 + i * 112);
            if (!controller || controller == buff::localcontroler) continue;

            // Проверяем команду этого игрока
            uint8_t enemyTeam = Read<uint8_t>(controller + offest::m_iTeamNum);

            // Если это союзник или команда не определена (0) — пропускаем
            if (enemyTeam == myTeam || enemyTeam == 0) continue;

            int pawnHandle = Read<int>(controller + offest::m_hPlayerPawn);
            int pawnIndex = pawnHandle & 0x7FFF;
            uintptr_t pawnChunk = Read<uintptr_t>(buff::EntityList + (pawnIndex >> 9) * 8 + 0x10);
            uintptr_t pawn = Read<uintptr_t>(pawnChunk + (pawnIndex & 0x1FF) * 112);

            if (!pawn) continue;
            // Проверка на HP (чтобы не светить трупы)
            int health = Read<int>(pawn + offest::m_iHealth);
            if (health <= 0 || health > 100) continue; {
                Vector3 origin = Read<Vector3>(pawn + offest::m_vOldOrigin);
             
                
            }

            Sleep(1);
        }
        Sleep(1);
    }
    CloseHandle(process);
}
