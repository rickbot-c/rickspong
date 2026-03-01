#pragma once
#include "raylib.h"
#include "Ball.h"
#include "Constants.h"

class Paddle {
public:
    Vector2 position;
    float   width        = 16.0f;
    float   height       = 100.0f;
    float   playerSpeed  = PLAYER_SPEED;
    float   aiSpeedFactor = AI_SPEED_BASE;
    float   squishY      = 1.0f;
    float   squishVelY   = 0.0f;
    float velY = 0.0f;   // updated each frame, used for spin transfer

    Paddle(float x, float y);

    void      UpdatePlayer(KeyboardKey up, KeyboardKey dn, float dt);
    void      UpdateAI(const Ball& ball, float dt);
    void      RecalcTarget(const Ball& ball);
    void      Bounce();
    void      Draw(Color col) const;
    Rectangle GetRect() const;

private:
    float aiTargetY = WIN_HEIGHT / 2.0f;
    float aiRecalcTimer = 0.0f;  // countdown to next target recalc

    float PredictBallY(const Ball& ball) const;
    void  Clamp();
    void  SpringSquish(float dt);
};

// ── Collision helpers ────────────────────────────────────────────────────────
bool BallHitsPaddle(const Ball& b, const Paddle& p);
void ResolvePaddleCollision(Ball& ball, Paddle& paddle, float pushDir);
float CalcAISpeedFactor(int playerScore, int aiScore);