#include "Player.h"
#include "Constants.h"
#include <algorithm>
#include <cmath>

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
    return 1400.0f; // placeholder, no ball speed upgrades yet
}

float Player::GetBonusMult() const {
    return upgLevels[UPG_MULTIPLIER] * 0.05f;
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