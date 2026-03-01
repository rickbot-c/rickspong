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

    position.x += velocity.x * dt;
    position.y += velocity.y * dt;

    if (position.y - radius <= 0) {
        position.y = radius;
        velocity.y = std::abs(velocity.y);
        SpawnHitParticles({ position.x, 0 }, { 180, 220, 255, 255 }, 8);
    }
    if (position.y + radius >= WIN_HEIGHT) {
        position.y = WIN_HEIGHT - radius;
        velocity.y = -std::abs(velocity.y);
        SpawnHitParticles({ position.x, (float)WIN_HEIGHT }, { 180, 220, 255, 255 }, 8);
    }
}

float Ball::GetSpeed() const {
    return sqrtf(velocity.x*velocity.x + velocity.y*velocity.y);
}

void Ball::Draw() const {
    float rx = radius * std::max(0.3f, squish);
    float ry = radius / std::max(0.3f, squish);
    DrawEllipse((int)position.x, (int)position.y, rx+3, ry+3, COL_OUTLINE);
    DrawEllipse((int)position.x, (int)position.y, rx,   ry,   COL_BALL);
    // Shine
    DrawEllipse((int)(position.x - rx*0.2f), (int)(position.y - ry*0.25f),
                rx*0.35f, ry*0.25f, { 255, 255, 220, 120 });
}