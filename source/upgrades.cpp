#include "Upgrades.h"
#include "Constants.h"
#include <algorithm>
#include <cmath>

const UpgradeDef UPGRADES[UPG_COUNT] = {
    { "TURBO LEGS",  "+15% player speed",      40.0f, 2.2f, 5, { 130, 255, 130, 255 } },
    { "SHIELD",      "1 free miss per round",  150.0f, 6.0f, 3, { 255, 200,  50, 255 } },
    { "COMBO BOOST", "+rally mult bonus",       50.0f, 3.5f, 5, { 255, 140,  60, 255 } },
};

float UpgradeCost(UpgradeID id, int currentLevel) {
    float c = UPGRADES[id].baseCost;
    for (int i = 0; i < currentLevel; i++) c *= UPGRADES[id].costScale;
    return c;
}

float CalcMultiplier(int hits, float bonus) {
    if (hits <= 0) return 1.0f;
    // Base: 1.25x per hit, bonus adds up to +0.5x per hit at max level
    float perHit = 1.25f + bonus * 0.5f;
    float m = powf(perHit, hits);
    return std::min(m, MULT_CAP);
}