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

    // Abilities
    bool  ownedAbilities[ABL_COUNT] = {};
    float abilityCooldowns[ABL_COUNT] = {};  // countdown timers
    KeyboardKey abilityKeys[ABL_COUNT] = { KEY_E, KEY_R };  // customizable keybinds

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

    // ── Abilities ─────────────────────────────────────────────────────────────
    bool  CanBuyAbility(AbilityID id) const;
    bool  BuyAbility(AbilityID id);     // returns false if can't afford / already owned
    bool  CanUseAbility(AbilityID id) const;
    void  UseAbility(AbilityID id);     // activates ability, starts cooldown
    void  UpdateAbilityCooldowns(float dt);

    float GetPlayerSpeedMult()  const;
    float GetBallMaxSpeed()     const;
    float GetBonusMult()        const;

    // ── Shield ───────────────────────────────────────────────────────────────
    // Called when player presses SPACE; deducts cost, sets shieldHP
    void  PrepareShields();
    bool  ConsumeShield();              // returns true if a shield was used

    // ── Save/Load ────────────────────────────────────────────────────────
    bool  SaveGame(const char* filename) const;
    bool  LoadGame(const char* filename);

private:
    float XPRequired(int forLevel) const;
};