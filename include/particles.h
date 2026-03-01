#pragma once
#include "raylib.h"
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Particles
// ─────────────────────────────────────────────────────────────────────────────
struct Particle {
    Vector2 pos, vel;
    float   life, maxLife, radius, rotation, rotSpeed;
    Color   col;
    bool    square;
};

void SpawnHitParticles(Vector2 pos, Color col, int count = 18);
void UpdateParticles(float dt);
void DrawParticles();

// ─────────────────────────────────────────────────────────────────────────────
// Floating text
// ─────────────────────────────────────────────────────────────────────────────
struct FloatText {
    Vector2     pos;
    std::string text;
    Color       col;
    float       life, maxLife;
    int         fs;
};

void SpawnFloatText(Vector2 pos, const char* text, Color col, int fs = 24);
void UpdateFloatTexts(float dt);
void DrawFloatTexts();

// ─────────────────────────────────────────────────────────────────────────────
// Screen shake
// ─────────────────────────────────────────────────────────────────────────────
void    TriggerShake(float strength, float dur);
Vector2 ShakeOffset(float dt);