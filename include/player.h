#pragma once
#include "Upgrades.h"

// ─────────────────────────────────────────────────────────────────────────────
// Player — owns all persistent state
// ─────────────────────────────────────────────────────────────────────────────
class Player {
public:
    // Economy
    float money     = 0.0f;

    // Leveling
    int   level     = 1;
    float xp        = 0.0f;
    float xpToNext  = 0.0f;   // filled in constructor

    // Upgrades
    int   upgLevels[UPG_COUNT] = {};

    // Per-round state
    int   shieldHP  = 0;

    Player();

    // ── Economy ──────────────────────────────────────────────────────────────
    void  AddMoney(float amount);
    void  LoseMoney(float amount);

    // ── Leveling ─────────────────────────────────────────────────────────────
    void  AddXP(float amount);          // returns true if levelled up (check via levelledUp flag)
    bool  levelledUp = false;           // set by AddXP, cleared by caller

    // ── Upgrades ─────────────────────────────────────────────────────────────
    bool  CanBuyUpgrade(UpgradeID id) const;
    bool  BuyUpgrade(UpgradeID id);     // returns false if can't afford / maxed

    float GetPlayerSpeedMult()  const;
    float GetBallMaxSpeed()     const;
    float GetBonusMult()        const;

    // ── Shield ───────────────────────────────────────────────────────────────
    // Called when player presses SPACE; deducts cost, sets shieldHP
    void  PrepareShields();
    bool  ConsumeShield();              // returns true if a shield was used

private:
    float XPRequired(int forLevel) const;
};