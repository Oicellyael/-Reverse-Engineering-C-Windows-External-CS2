#include "help.h"
#include <fstream>
#include <string>

HANDLE process = nullptr;
IDXGISwapChain* swapchain = nullptr;
ID3D11Device* dev = nullptr;
ID3D11DeviceContext* pContext = nullptr;
ID3D11RenderTargetView* pRenderTargetView = nullptr;
namespace buff {
    uintptr_t clientdllbase = 0;
    uintptr_t localcontroler = 0;
    uintptr_t localPawn = 0;
    uintptr_t EntityList = 0;
    uintptr_t Controller = 0;
    uintptr_t chunk0 = 0;
    bool glow=false;
   // bool radar=false;
}

namespace trigger {
    int scope = 0;
    uintptr_t scopecontroller = 0;
    uintptr_t scopechunkbase = 0;
    uint8_t Team = 0;
    uint8_t EnemyTeam = 0;
    bool bot = false;
}

namespace rts {
    int shotFired = 0;
    bool recoil = false;
}

void SaveConfig() {
    std::ofstream file("config.ini");
    file << "triggerbot=" << trigger::bot << "\n";
    file << "rcs=" << rts::recoil << "\n";
    file.close();
}

void LoadConfig() {
    std::ifstream file("config.ini");
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("triggerbot=") == 0) trigger::bot = std::stoi(line.substr(11));
        if (line.find("rcs=") == 0) rts::recoil = std::stoi(line.substr(4));
    }
}

uintptr_t GetModuleBase(DWORD pid, const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    MODULEENTRY32 mod;
    mod.dwSize = sizeof(mod);
    if (Module32First(snap, &mod)) {
        do {
            if (_wcsicmp(mod.szModule, name) == 0) {
                CloseHandle(snap);
                return reinterpret_cast<uintptr_t>(mod.modBaseAddr);
            }
        } while (Module32Next(snap, &mod));
    }
    CloseHandle(snap);
    return 0;
}
void ClampAngles(Vector2& angles) {
    // Ограничиваем Pitch (вверх/вниз)
    if (angles.x > 89.0f) angles.x = 89.0f;
    if (angles.x < -89.0f) angles.x = -89.0f;

    // Ограничиваем Yaw (лево/право)
    while (angles.y > 180.0f) angles.y -= 360.0f;
    while (angles.y < -180.0f) angles.y += 360.0f;
}
