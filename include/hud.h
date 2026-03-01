#pragma once
#include "raylib.h"
#include "Player.h"

// ─────────────────────────────────────────────────────────────────────────────
// Low-level draw helpers
// ─────────────────────────────────────────────────────────────────────────────
void DrawOutlinedRect  (Rectangle r, float roundness, Color fill, Color outline, float thick = 3.0f);
void DrawOutlinedCircle(Vector2 pos, float r, Color fill, Color outline, float thick = 2.5f);
void DrawOutlinedText  (const char* text, int x, int y, int fs, Color col, int ow = 2);

// ─────────────────────────────────────────────────────────────────────────────
// HUD elements
// ─────────────────────────────────────────────────────────────────────────────
void DrawStatPanel    (int x, int y, const char* label, const char* value,
                       Color accent, bool highlight = false);
void DrawDifficultyBar(float factor);
void DrawXPBar        (const Player& player);
void DrawShieldPips   (int shieldHP, int upgShieldLevel, float playerMoney, int state);

// ─────────────────────────────────────────────────────────────────────────────
// Upgrade shop overlay
// ─────────────────────────────────────────────────────────────────────────────
void DrawUpgradeShop(const Player& player, int& hovered);