#pragma once
#include "raylib.h"

// ─────────────────────────────────────────────────────────────────────────────
// Upgrades
// ─────────────────────────────────────────────────────────────────────────────
enum UpgradeID {
    UPG_PLAYER_SPEED,
    UPG_SHIELD,
    UPG_MULTIPLIER,
    UPG_COUNT
};

enum AbilityID {
    ABL_SPEED,
    ABL_PULL,
    ABL_COUNT
};

struct UpgradeDef {
    const char* name;
    const char* desc;
    float       baseCost;
    float       costScale;
    int         maxLevel;
    Color       accent;
};

struct AbilityDef {
    const char* name;
    const char* desc;
    float       cost;
    float       maxCooldown;
    Color       accent;
};

extern const UpgradeDef UPGRADES[UPG_COUNT];
extern const AbilityDef ABILITIES[ABL_COUNT];

float UpgradeCost(UpgradeID id, int currentLevel);
float CalcMultiplier(int hits, float bonus);