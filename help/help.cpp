#define STB_IMAGE_IMPLEMENTATION // СНАЧАЛА ОПРЕДЕЛЯЕМ
#include "../stb_image.h"
#include "help.h"
#include <fstream>


HANDLE process = nullptr;
IDXGISwapChain* swapchain = nullptr;
ID3D11Device* dev = nullptr;
ID3D11DeviceContext* pContext = nullptr;
ID3D11RenderTargetView* pRenderTargetView = nullptr;
ID3D11ShaderResourceView* mapTexture = nullptr;

std::string currentMapName = "mirage";
std::map<std::string, Maps> myMap = {
    { "mirage",  { -3230.0f, 1713.0f, 5.0f, nullptr } },
    { "dust2",   { -2476.0f, 3239.0f, 4.4f, nullptr } },
    { "inferno", {- 2087.0f, 3870.0f, 4.9f, nullptr } },
    { "overpass",{- 4831.0f, 1781.0f, 5.2f, nullptr } },
    { "ancient", { -2976.0f, 2176.0f, 5.0f, nullptr } }
};

namespace buff {
    uintptr_t clientdllbase = 0;
	uintptr_t enginedllbase = 0;
    uintptr_t localcontroler = 0;
    uintptr_t localPawn = 0;
    uintptr_t EntityList = 0;
    uintptr_t Controller = 0;
    uintptr_t chunk0 = 0;
    bool glow=false;
    bool radar = false;    // По умолчанию включен
    bool showMenu = false;
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

void LoadAllMap() {
    for (auto& pair : myMap) {
            std::string filename = "maps/" + pair.first + ".png";
            int width, height, channels;
            unsigned char* pixels = stbi_load(filename.c_str(), &width, &height, &channels, 4);

            if (!pixels) {
                std::cout << "[-] Failed to load image: " << filename << std::endl;
                continue;
            }

            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width = (UINT)width;
            desc.Height = (UINT)height;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

            D3D11_SUBRESOURCE_DATA initData = { pixels, (UINT)(width * 4), 0 };
            ID3D11Texture2D* pTexture = nullptr;
            dev->CreateTexture2D(&desc, &initData, &pTexture);

            if (pTexture) {
                dev->CreateShaderResourceView(pTexture, nullptr, &pair.second.texture);
                pTexture->Release();
                std::cout << "[+] Loaded: " << pair.first << std::endl;
            }
            stbi_image_free(pixels);
        }
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

void DrawEnemies(ImDrawList* DrawList, float cX, float cY, float zoom, float s, float c, float my_px, float my_py, Maps& mapData, uint8_t myTeam) {
    // 2. Рисуем ВРАГОВ
    for (int i = 0; i < 64; i++) {
        uintptr_t ctrl = Read<uintptr_t>(buff::chunk0 + i * 112);
        if (!ctrl || ctrl == buff::localcontroler) continue;
        if (Read<uint8_t>(ctrl + offest::m_iTeamNum) == myTeam) continue;

        int pawnHandle = Read<int>(ctrl + offest::m_hPlayerPawn);
        int pIdx = pawnHandle & 0x7FFF;
        uintptr_t pChunk = Read<uintptr_t>(buff::EntityList + (pIdx >> 9) * 8 + 0x10);
        uintptr_t pawn = Read<uintptr_t>(pChunk + (pIdx & 0x1FF) * 112);

        if (!pawn || Read<int>(pawn + offest::m_iHealth) <= 0) continue;

        Vector3 enPos = Read<Vector3>(pawn + offest::m_vOldOrigin);
        float dx = ((enPos.x - mapData.map_origin_x) / mapData.map_scale - my_px) * zoom;
        float dy = ((mapData.map_origin_y - enPos.y) / mapData.map_scale - my_py) * zoom;

        float rx = c * dx - s * dy;
        float ry = s * dx + c * dy;

        DrawList->AddCircleFilled(ImVec2(cX + rx, cY + ry), 4.5f, ImColor(255, 0, 0));
        DrawList->AddCircle(ImVec2(cX + rx, cY + ry), 5.5f, ImColor(0, 0, 0));
    }

    // Твой маркер
    DrawList->AddCircleFilled(ImVec2(cX, cY), 6, ImColor(0, 255, 0));
    DrawList->AddCircle(ImVec2(cX, cY), 7, ImColor(255, 255, 255));

    DrawList->PopClipRect();
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

Vector2 oldPunch = { 0, 0 };
INPUT clicks[2] = {};

void RunTriggerbot() {
    trigger::scope = Read<int>(buff::localPawn + offest::m_iIDEntIndex);
    if (trigger::scope != -1) {
        trigger::scopechunkbase = Read<uintptr_t>(buff::EntityList + (trigger::scope >> 9) * 8 + 0x10);
        trigger::scopecontroller = Read<uintptr_t>(trigger::scopechunkbase + (trigger::scope & 0x1FF) * 112);

        trigger::Team = Read<uint8_t>(buff::localcontroler + offest::m_iTeamNum);
        trigger::EnemyTeam = Read<uint8_t>(trigger::scopecontroller + offest::m_iTeamNum);

        if (trigger::EnemyTeam != trigger::Team && trigger::EnemyTeam != 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
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
        if(!buff::glow || !buff::chunk0 || !buff::localcontroler) {
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
                Write<int>(glowBase + 0x40, 0xFF00FFFF); // желный для врагов
                Write<int>(glowBase + 0x30, 1);
                Write<bool>(glowBase + offest::m_bGlowing, true);
            }
        }

    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(hwnd, msg, wp, lp);
}
MSG msg;

DWORD WINAPI RenderThread(LPVOID) {
    while (true) {
        if (!buff::localPawn || !buff::localcontroler || !buff::chunk0) {
            Sleep(10);
            continue;
        }
        // Читаем данные твоего игрока
        Vector3 localOrigin = Read<Vector3>(buff::localPawn + offest::m_vOldOrigin);
        Vector2 viewAngles = Read<Vector2>(buff::clientdllbase + offest::dwViewAngles);
        uint8_t myTeam = Read<uint8_t>(buff::localcontroler + offest::m_iTeamNum);

        // Начинаем кадр ImGui
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        float clearColor[4] = { 0, 0, 0, 0 };
        pContext->ClearRenderTargetView(pRenderTargetView, clearColor);

        if (buff::radar && myMap.count(currentMapName)) {
            auto& mapData = myMap[currentMapName];
            ImDrawList* DrawList = ImGui::GetBackgroundDrawList();

            // Математика поворота
            float angle = -viewAngles.y * (3.14159f / 180.0f);
            float s = sin(angle), c = cos(angle);

            // Твоя позиция в пикселях карты
            float my_px = (localOrigin.x - mapData.map_origin_x) / mapData.map_scale;
            float my_py = (mapData.map_origin_y - localOrigin.y) / mapData.map_scale;

            // Параметры радара
            float rSize = 400.0f;
            float cX = rSize / 2.0f;
            float cY = rSize - 70.0f; // Центр смещен вниз
            float zoom = 0.4f;

            DrawList->PushClipRect(ImVec2(0, 0), ImVec2(rSize, rSize), true);
            DrawList->AddRectFilled(ImVec2(0, 0), ImVec2(rSize, rSize), ImColor(0, 0, 0, 150));

            // 1. Рисуем КАРТУ (1024x1024 - стандарт для радаров CS2)
            if (mapData.texture) {
                float sz = 1024.0f;
                ImVec2 corners[4] = {
                    { (0 - my_px) * zoom, (0 - my_py) * zoom },
                    { (sz - my_px) * zoom, (0 - my_py) * zoom },
                    { (sz - my_px) * zoom, (sz - my_py) * zoom },
                    { (0 - my_px) * zoom, (sz - my_py) * zoom }
                };

                ImVec2 rot[4];
                for (int i = 0; i < 4; i++) {
                    float rx = c * corners[i].x - s * corners[i].y;
                    float ry = s * corners[i].x + c * corners[i].y;
                    rot[i] = ImVec2(cX + rx, cY + ry);
                }
                DrawList->AddImageQuad((ImTextureID)mapData.texture, rot[0], rot[1], rot[2], rot[3]);
            }

            // 2. Рисуем ВРАГОВ
            DrawEnemies(DrawList, cX, cY, zoom, s, c, my_px, my_py, mapData, myTeam);
        }

        ImGui::Render();
        pContext->OMSetRenderTargets(1, &pRenderTargetView, NULL);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        swapchain->Present(1, 0);
    }
}
