#pragma comment(lib, "d3d11.lib")
#include "help/help.h"
#include <iostream>
#include <string>
#include <chrono>
#include <d3d11.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

using namespace std;

// Функция для очистки пути карты (оставляем в майн для удобства)
std::string CleanMapName(std::string rawName) {
    size_t lastSlash = rawName.find_last_of("\\/");
    if (lastSlash != std::string::npos) rawName = rawName.substr(lastSlash + 1);
    size_t lastDot = rawName.find_last_of(".");
    if (lastDot != std::string::npos) rawName = rawName.substr(0, lastDot);
    if (rawName.find("de_") == 0) rawName = rawName.substr(3);
    return rawName;
}

int main() {
    // 1. Инициализация процесса
    HWND window = FindWindowA(NULL, "Counter-Strike 2");
    if (!window) { cout << "Waiting for CS2..." << endl; return 0; }

    DWORD pid;
    GetWindowThreadProcessId(window, &pid);
    process = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pid);

    buff::clientdllbase = GetModuleBase(pid, L"client.dll");
    buff::enginedllbase = GetModuleBase(pid, L"engine2.dll");

    if (!buff::clientdllbase || !buff::enginedllbase){
        cout << "Failed to get modules!" << endl;
        return 0;
    }

    // 2. Настройка управления (мышь)
    clicks[0].type = clicks[1].type = INPUT_MOUSE;
    clicks[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    clicks[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

    // 3. Создание окна оверлея
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"RadarClass", NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT, L"RadarClass", L"Radar", WS_POPUP, 0, 0, 400, 400, NULL, NULL, wc.hInstance, NULL);
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(hwnd, SW_SHOW);

    // 4. Инициализация DirectX 11
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.Width = 400; // Под размер окна
    sd.BufferDesc.Height = 400;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &sd, &swapchain, &dev, NULL, &pContext);

    ID3D11Texture2D* pBackBuffer;
    swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    dev->CreateRenderTargetView(pBackBuffer, NULL, &pRenderTargetView);
    pBackBuffer->Release();

    // --- ГЛАВНОЕ: Загружаем все текстуры карт в память видеокарты ---
    LoadAllMap();

    // 5. Инициализация ImGui
    ImGui::CreateContext();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(dev, pContext);

    // 6. Запуск рабочих потоков
    CreateThread(nullptr, 0, GlowThread, nullptr, 0, nullptr);
    CreateThread(nullptr, 0, RenderThread, nullptr, 0, nullptr);

    LoadConfig();
    cout << "Cheat Started. Ready to go!" << endl;

    // --- ГЛАВНЫЙ ЦИКЛ ---
    while (!GetAsyncKeyState(VK_DELETE)) {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // --- УМНОЕ ОПРЕДЕЛЕНИЕ КАРТЫ (Раз в 2 секунды) ---
        static auto lastMapCheck = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastMapCheck).count() >= 2) {
            uintptr_t ptr;
            // Цепочка: engine2.dll + 0x909CF8 -> 0x38 -> 0x1A8
            if (ReadProcessMemory(process, (LPCVOID)(buff::enginedllbase + 0x909CF8), &ptr, 8, NULL) && ptr) {
                if (ReadProcessMemory(process, (LPCVOID)(ptr + 0x38), &ptr, 8, NULL) && ptr) {
                    if (ReadProcessMemory(process, (LPCVOID)(ptr + 0x1A8), &ptr, 8, NULL) && ptr) {
                        char rawName[64];
                        if (ReadProcessMemory(process, (LPCVOID)ptr, &rawName, 64, NULL)) {
                            std::string newMap = CleanMapName(rawName);
                            if (newMap != currentMapName) {
                                currentMapName = newMap;
                                cout << "[MAP] Switched to: " << currentMapName << endl;
                            }
                        }
                    }
                }
            }
            lastMapCheck = now;
        }
        // --- ОБНОВЛЕНИЕ ДАННЫХ ИГРОКОВ ---
        buff::localcontroler = Read<uintptr_t>(buff::clientdllbase + offest::dwLocalPlayerController);
        buff::localPawn = Read<uintptr_t>(buff::clientdllbase + offest::dwLocalPlayerPawn);
        buff::EntityList = Read<uintptr_t>(buff::clientdllbase + offest::dwEntityList);
        buff::chunk0 = Read<uintptr_t>(buff::EntityList + 0x10);

        // Обработка горячих клавиш
        if (GetAsyncKeyState(VK_F5) & 1) { SaveConfig(); cout << "Config Saved!" << endl; }
		if (GetAsyncKeyState(VK_XBUTTON1) & 1) trigger::bot = !trigger::bot, cout << "Triggerbot: " << (trigger::bot ? "ON" : "OFF") << endl;
		if (GetAsyncKeyState(VK_NUMPAD1) & 1) rts::recoil = !rts::recoil,cout << "RCS: " << (rts::recoil ? "ON" : "OFF") << endl;
		if (GetAsyncKeyState(VK_NUMPAD2) & 1) buff::glow = !buff::glow, cout << "Glow: " << (buff::glow ? "ON" : "OFF") << endl;
        if (GetAsyncKeyState(VK_NUMPAD3) & 1) buff::radar = !buff::radar;

        // Запуск логики
        if (trigger::bot) RunTriggerbot();
        if (rts::recoil)  RunRCS();

        Sleep(1);
    }
    // Завершение работы
    CloseHandle(process);
    return 0;
}
