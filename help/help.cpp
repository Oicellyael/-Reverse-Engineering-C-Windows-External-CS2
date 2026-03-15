#include "help.h"
HANDLE process = nullptr;
namespace buff {
    uintptr_t clientdllbase = 0;
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
