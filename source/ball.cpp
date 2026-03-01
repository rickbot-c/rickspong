#include "Ball.h"
#include "Constants.h"
#include "Particles.h"
#include <cmath>
#include <algorithm>

Ball::Ball() {
    position = { WIN_WIDTH/2.0f, WIN_HEIGHT/2.0f };
    velocity = { 0, 0 };
}

void Ball::Reset(int dir) {
    position  = { WIN_WIDTH/2.0f, WIN_HEIGHT/2.0f };
    velocity  = { BALL_SPEED_INITIAL * dir,
                  BALL_SPEED_INITIAL * (GetRandomValue(0,1) ? 1.0f : -1.0f) };
    spin      = 0.0f;              // clear any leftover rotation
    squish    = 1.0f;
    squishVel = 0.0f;
}

void Ball::CapSpeed(float maxSpd) {
    float spd = GetSpeed();
    if (spd > maxSpd) {
        velocity.x = velocity.x / spd * maxSpd;
        velocity.y = velocity.y / spd * maxSpd;
    }
}

void Ball::Update(float dt) {
    // Spring squish
    float a   = -320.0f*(squish-1.0f) - 12.0f*squishVel;
    squishVel += a * dt;
    squish    += squishVel * dt;

    // Magnus effect
    float spd = GetSpeed();
    if (spd > 0.001f && std::abs(spin) > 0.1f) {
        float dx = velocity.x / spd;
        float dy = velocity.y / spd;

        // Increased from 1.0 to 4.0 — actually visible now
        float perpX = -dy * spin * dt * 4.0f;
        float perpY =  dx * spin * dt * 4.0f;

        velocity.x += perpX;
        velocity.y += perpY;

        // Renormalise — spin changes direction only, not speed
        float newSpd = GetSpeed();
        if (newSpd > 0.001f) {
            velocity.x = velocity.x / newSpd * spd;
            velocity.y = velocity.y / newSpd * spd;
        }
    }

    // Slow decay — spin should last most of the ball's flight
    spin *= powf(0.02f, dt);  // was 0.15f, now decays to ~2% over 1 second

    position.x += velocity.x * dt;
    position.y += velocity.y * dt;

    if (position.y - radius <= 0) {
        position.y = radius;
        velocity.y = std::abs(velocity.y);
        spin = -spin * 0.5f;
        SpawnHitParticles({ position.x, 0 }, { 180, 220, 255, 255 }, 8);
    }
    if (position.y + radius >= WIN_HEIGHT) {
        position.y = WIN_HEIGHT - radius;
        velocity.y = -std::abs(velocity.y);
        spin = -spin * 0.5f;
        SpawnHitParticles({ position.x, (float)WIN_HEIGHT }, { 180, 220, 255, 255 }, 8);
    }
}
float Ball::GetSpeed() const {
    return sqrtf(velocity.x*velocity.x + velocity.y*velocity.y);
}

void Ball::Draw() const {
    float rx = radius * std::max(0.3f, squish);
    float ry = radius / std::max(0.3f, squish);

    // Spin tint: blue = topspin, red = backspin
    float spinT = std::max(-1.0f, std::min(1.0f, spin / 500.0f));
    Color ballCol = spinT > 0
        ? Color{ 255, (unsigned char)(240 - spinT*80), (unsigned char)(100 - spinT*100), 255 }
        : Color{ (unsigned char)(255 + spinT*80), 240, (unsigned char)(100 - spinT*100), 255 };

    DrawEllipse((int)position.x, (int)position.y, rx+3, ry+3, COL_OUTLINE);
    DrawEllipse((int)position.x, (int)position.y, rx,   ry,   ballCol);
    DrawEllipse((int)(position.x - rx*0.2f), (int)(position.y - ry*0.25f),
                rx*0.35f, ry*0.25f, { 255, 255, 220, 120 });
}