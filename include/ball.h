#pragma once
#include "raylib.h"

class Ball {
public:
    Vector2 position, velocity;
    float   radius    = 12.0f;
    float   squish    = 1.0f;
    float   squishVel = 0.0f;

    Ball();

    void  Reset(int dir);
    void  CapSpeed(float maxSpd);
    void  Update(float dt);
    float GetSpeed() const;
    void  Draw()    const;
};