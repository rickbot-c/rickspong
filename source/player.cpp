#include "Player.h"
#include "Constants.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

Player::Player() {
    xpToNext = XPRequired(level);
}

// ─────────────────────────────────────────────────────────────────────────────
// Economy
// ─────────────────────────────────────────────────────────────────────────────
void Player::AddMoney(float amount) {
    money += amount;
}

void Player::LoseMoney(float amount) {
    money = std::max(MONEY_FLOOR, money - amount);
}

// ─────────────────────────────────────────────────────────────────────────────
// Leveling
// ─────────────────────────────────────────────────────────────────────────────
float Player::XPRequired(int forLevel) const {
    // Each level needs XP_BASE * XP_SCALE^(level-1)
    float req = XP_BASE;
    for (int i = 1; i < forLevel; i++) req *= XP_SCALE;
    return req;
}

void Player::AddXP(float amount) {
    levelledUp = false;
    xp += amount;
    while (xp >= xpToNext) {
        xp      -= xpToNext;
        level++;
        xpToNext = XPRequired(level);
        levelledUp = true;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Upgrades
// ─────────────────────────────────────────────────────────────────────────────
bool Player::CanBuyUpgrade(UpgradeID id) const {
    if (upgLevels[id] >= UPGRADES[id].maxLevel) return false;
    return money >= UpgradeCost(id, upgLevels[id]);
}

bool Player::BuyUpgrade(UpgradeID id) {
    if (!CanBuyUpgrade(id)) return false;
    money -= UpgradeCost(id, upgLevels[id]);
    upgLevels[id]++;
    return true;
}

float Player::GetPlayerSpeedMult() const {
    return 1.0f + upgLevels[UPG_PLAYER_SPEED] * 0.15f;
}

float Player::GetBallMaxSpeed() const {
    return 3802.0f; // placeholder, no ball speed upgrades yet
}

float Player::GetBonusMult() const {
    return upgLevels[UPG_MULTIPLIER] * 0.05f;
}

// ─────────────────────────────────────────────────────────────────────────────
// Abilities
// ─────────────────────────────────────────────────────────────────────────────
bool Player::CanBuyAbility(AbilityID id) const {
    if (ownedAbilities[id]) return false;  // already own it
    return money >= ABILITIES[id].cost;
}

bool Player::BuyAbility(AbilityID id) {
    if (!CanBuyAbility(id)) return false;
    money -= ABILITIES[id].cost;
    ownedAbilities[id] = true;
    abilityCooldowns[id] = 0.0f;  // ready to use immediately
    return true;
}

bool Player::CanUseAbility(AbilityID id) const {
    return ownedAbilities[id] && abilityCooldowns[id] <= 0.0f;
}

void Player::UseAbility(AbilityID id) {
    if (CanUseAbility(id)) {
        abilityCooldowns[id] = ABILITIES[id].maxCooldown;
    }
}

void Player::UpdateAbilityCooldowns(float dt) {
    for (int i = 0; i < ABL_COUNT; i++) {
        if (abilityCooldowns[i] > 0.0f) {
            abilityCooldowns[i] -= dt;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Shield
// ─────────────────────────────────────────────────────────────────────────────
void Player::PrepareShields() {
    int affordable = 0;
    for (int i = 0; i < upgLevels[UPG_SHIELD]; i++) {
        if (money >= SHIELD_USE_COST * (i + 1))
            affordable = i + 1;
    }
    shieldHP = affordable;
    money   -= SHIELD_USE_COST * affordable;
}

bool Player::ConsumeShield() {
    if (shieldHP <= 0) return false;
    shieldHP--;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Save/Load
// ─────────────────────────────────────────────────────────────────────────────
bool Player::SaveGame(const char* filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) return false;

    // Save basic stats
    file << money << "\n";
    file << level << "\n";
    file << xp << "\n";

    // Save upgrades
    for (int i = 0; i < UPG_COUNT; i++) {
        file << upgLevels[i] << " ";
    }
    file << "\n";

    // Save abilities
    for (int i = 0; i < ABL_COUNT; i++) {
        file << (ownedAbilities[i] ? 1 : 0) << " ";
    }
    file << "\n";

    // Save ability keybinds (as int values)
    for (int i = 0; i < ABL_COUNT; i++) {
        file << (int)abilityKeys[i] << " ";
    }
    file << "\n";

    file.close();
    return true;
}

bool Player::LoadGame(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    // Load basic stats
    file >> money >> level >> xp;
    xpToNext = XPRequired(level);

    // Load upgrades
    for (int i = 0; i < UPG_COUNT; i++) {
        file >> upgLevels[i];
    }

    // Load abilities
    for (int i = 0; i < ABL_COUNT; i++) {
        int owned;
        file >> owned;
        ownedAbilities[i] = (owned != 0);
        abilityCooldowns[i] = 0.0f;  // Reset cooldowns on load
    }

    // Load ability keybinds
    for (int i = 0; i < ABL_COUNT; i++) {
        int key;
        file >> key;
        abilityKeys[i] = (KeyboardKey)key;
    }

    file.close();
    return true;
}