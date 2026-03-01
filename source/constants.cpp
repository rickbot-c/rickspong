#pragma once
#include "raylib.h"

// ─────────────────────────────────────────────────────────────────────────────
// Window / gameplay constants
// ─────────────────────────────────────────────────────────────────────────────
constexpr int   WIN_WIDTH           = 800;
constexpr int   WIN_HEIGHT          = 650;
constexpr float BALL_SPEED_INITIAL  = 280.0f;
constexpr float BALL_SPEED_INC      = 0.12f;
constexpr float PLAYER_SPEED        = 380.0f;

// AI difficulty
constexpr float AI_SPEED_MIN  = 0.50f;
constexpr float AI_SPEED_MAX  = 3.6f;
constexpr float AI_SPEED_BASE = 1.0f;
constexpr int   SCORE_CLAMP   = 3;

// Economy
constexpr float MULTIPLIER_PER_HIT = 1.15f;
constexpr float BASE_ROUND_REWARD  = 12.0f;
constexpr float BASE_ROUND_LOSS    = 18.0f;
constexpr float MONEY_FLOOR        = -500.0f;
constexpr float MULT_CAP           = 8.0f;
constexpr float SHIELD_USE_COST    = 30.0f;

// Leveling
constexpr float XP_BASE            = 100.0f;   // XP needed for level 1→2
constexpr float XP_SCALE           = 1.45f;    // multiplier per level
constexpr float XP_WIN_BASE        = 30.0f;    // XP awarded for winning a round
constexpr float XP_RALLY_BONUS     = 2.5f;     // extra XP per hit in the rally

// Palette
constexpr Color COL_BG      = {  18,  20,  40, 255 };
constexpr Color COL_PLAYER  = {  80, 200, 255, 255 };
constexpr Color COL_AI      = { 255,  90,  90, 255 };
constexpr Color COL_BALL    = { 255, 240, 100, 255 };
constexpr Color COL_OUTLINE = {  10,  10,  20, 255 };
constexpr Color COL_GOLD    = { 255, 215,  50, 255 };
constexpr Color COL_GREEN   = {  80, 230, 120, 255 };
constexpr Color COL_RED     = { 255,  80,  80, 255 };
constexpr Color COL_XP      = {  80, 160, 255, 255 };
