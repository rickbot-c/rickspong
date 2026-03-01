#pragma once
#include "raylib.h"

// ─────────────────────────────────────────────────────────────────────────────
// Upgrades
// ─────────────────────────────────────────────────────────────────────────────
enum UpgradeID {
    UPG_PLAYER_SPEED,
    UPG_BALL_SLOW,
    UPG_SHIELD,
    UPG_MULTIPLIER,
    UPG_COUNT
};

struct UpgradeDef {
    const char* name;
    const char* desc;
    float       baseCost;
    float       costScale;
    int         maxLevel;
    Color       accent;
};

extern const UpgradeDef UPGRADES[UPG_COUNT];

float UpgradeCost(UpgradeID id, int currentLevel);
float CalcMultiplier(int hits, float bonus);