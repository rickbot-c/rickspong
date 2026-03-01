#include "Particles.h"
#include "Constants.h"
#include <algorithm>
#include <cmath>

static std::vector<Particle>  gParticles;
static std::vector<FloatText> gFloatTexts;
static float gShakeTimer    = 0.0f;
static float gShakeStrength = 0.0f;

// ─────────────────────────────────────────────────────────────────────────────
// Particles
// ─────────────────────────────────────────────────────────────────────────────
void SpawnHitParticles(Vector2 pos, Color col, int count) {
    for (int i = 0; i < count; i++) {
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float spd   = (float)GetRandomValue(60, 280);
        float life  = GetRandomValue(25, 65) / 100.0f;
        Color c = col; c.a = 255;
        gParticles.push_back({ pos,
            { cosf(angle)*spd, sinf(angle)*spd },
            life, life, (float)GetRandomValue(3, 9),
            (float)GetRandomValue(0, 360),
            (float)GetRandomValue(-400, 400),
            c, (bool)GetRandomValue(0, 1) });
    }
}

void UpdateParticles(float dt) {
    for (auto& p : gParticles) {
        p.pos.x    += p.vel.x * dt;
        p.pos.y    += p.vel.y * dt;
        p.vel.y    += 320.0f * dt;
        p.rotation += p.rotSpeed * dt;
        p.life     -= dt;
        p.radius   *= powf(0.407f, dt);   // framerate-independent decay (~0.985^60)
    }
    gParticles.erase(
        std::remove_if(gParticles.begin(), gParticles.end(),
            [](const Particle& p){ return p.life <= 0.0f; }),
        gParticles.end());
}

void DrawParticles() {
    for (const auto& p : gParticles) {
        Color c = p.col;
        c.a = (unsigned char)((p.life / p.maxLife) * 255);
        if (p.square) {
            DrawRectanglePro(
                { p.pos.x - p.radius, p.pos.y - p.radius, p.radius*2, p.radius*2 },
                { p.radius, p.radius }, p.rotation, c);
        } else {
            DrawCircleV(p.pos, p.radius, c);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Floating text
// ─────────────────────────────────────────────────────────────────────────────
void SpawnFloatText(Vector2 pos, const char* text, Color col, int fs) {
    gFloatTexts.push_back({ pos, std::string(text), col, 1.4f, 1.4f, fs });
}

void UpdateFloatTexts(float dt) {
    for (auto& f : gFloatTexts) {
        f.pos.y -= 55.0f * dt;
        f.life  -= dt;
    }
    gFloatTexts.erase(
        std::remove_if(gFloatTexts.begin(), gFloatTexts.end(),
            [](const FloatText& f){ return f.life <= 0.0f; }),
        gFloatTexts.end());
}

void DrawFloatTexts() {
    for (const auto& f : gFloatTexts) {
        float t = f.life / f.maxLife;
        float a = (t < 0.2f) ? (t / 0.2f)
                : (t > 0.85f) ? ((1.0f-t)/0.15f*0.6f+0.4f)
                : 1.0f;
        Color c = f.col;
        c.a = (unsigned char)(a * 255);
        int w = MeasureText(f.text.c_str(), f.fs);
        for (int ox=-2; ox<=2; ox++) for (int oy=-2; oy<=2; oy++) {
            if (ox==0 && oy==0) continue;
            Color oc = COL_OUTLINE; oc.a = c.a;
            DrawText(f.text.c_str(), (int)(f.pos.x - w/2)+ox, (int)f.pos.y+oy, f.fs, oc);
        }
        DrawText(f.text.c_str(), (int)(f.pos.x - w/2), (int)f.pos.y, f.fs, c);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Screen shake
// ─────────────────────────────────────────────────────────────────────────────
void TriggerShake(float strength, float dur) {
    gShakeStrength = strength;
    gShakeTimer    = dur;
}

Vector2 ShakeOffset(float dt) {
    if (gShakeTimer <= 0.0f) return { 0, 0 };
    gShakeTimer -= dt;
    float s = gShakeStrength * std::min(gShakeTimer / 0.12f, 1.0f);
    return { (float)GetRandomValue(-1, 1)*s, (float)GetRandomValue(-1, 1)*s };
}