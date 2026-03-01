#include "Paddle.h"
#include "Particles.h"
#include <cmath>
#include <algorithm>

Paddle::Paddle(float x, float y) : position({ x, y }) {}

void Paddle::UpdatePlayer(KeyboardKey up, KeyboardKey dn, float dt) {
    // track vertical velocity so spin can be transferred to the ball
    float oldY = position.y;

    if (IsKeyDown(up)) position.y -= playerSpeed * dt;
    if (IsKeyDown(dn)) position.y += playerSpeed * dt;

    // simple finite difference, dt should never be zero
    velY = (position.y - oldY) / dt;

    Clamp();
    SpringSquish(dt);
}

void Paddle::UpdateAI(const Ball& ball, float dt) {
    float oldY = position.y;

    float target = (ball.velocity.x > 0) ? aiTargetY : WIN_HEIGHT/2.0f;
    if (ball.velocity.x <= 0) aiTargetY = WIN_HEIGHT/2.0f;
    float centre = position.y + height/2.0f;
    float diff   = target - centre;
    float spd    = PLAYER_SPEED * aiSpeedFactor * dt;
    float move   = std::min(std::abs(diff), spd);
    if (diff < -1.0f) position.y -= move;
    if (diff >  1.0f) position.y += move;

    // update velocity after moving
    velY = (position.y - oldY) / dt;

    Clamp();
    SpringSquish(dt);
}

void Paddle::RecalcTarget(const Ball& ball) {
    if (ball.velocity.x <= 0) { aiTargetY = WIN_HEIGHT/2.0f; return; }
    float pred     = PredictBallY(ball);
    float errRange = 10.0f + (ball.GetSpeed() - BALL_SPEED_INITIAL) * 0.015f;
    errRange = std::min(errRange, height * 0.72f);
    float t = std::max(0.0f, std::min(1.0f,
        (aiSpeedFactor - AI_SPEED_MIN) / (AI_SPEED_MAX - AI_SPEED_MIN)));
    errRange *= (0.02f + 0.98f*(1.0f - t));
    aiTargetY = pred + (GetRandomValue(-100, 100)/100.0f) * errRange;
}

void Paddle::Bounce() { squishY = 0.55f; squishVelY = 0.0f; }

void Paddle::Draw(Color col) const {
    float h  = height * std::max(0.3f, squishY);
    float cy = position.y + height/2.0f - h/2.0f;
    // Outline
    Rectangle outer = { position.x-3, cy-3, width+6, h+6 };
    DrawRectangleRounded(outer, 0.5f, 8, COL_OUTLINE);
    DrawRectangleRounded({ position.x, cy, width, h }, 0.5f, 8, col);
    // Shine strip
    DrawRectangleRounded({ position.x+3, cy+h*0.12f, width-6, h*0.35f },
                         0.5f, 4, Fade(WHITE, 0.2f));
}

Rectangle Paddle::GetRect() const {
    return { position.x, position.y, width, height };
}

float Paddle::PredictBallY(const Ball& ball) const {
    float vx = ball.velocity.x;
    if (vx <= 0) return ball.position.y;
    float t  = (position.x - ball.position.x) / vx;
    float py = ball.position.y + ball.velocity.y * t;
    float mn = ball.radius, mx = WIN_HEIGHT - ball.radius, rng = mx - mn;
    py -= mn;
    if (rng > 0) {
        py = std::fmod(std::abs(py), rng*2.0f);
        if (py > rng) py = rng*2.0f - py;
    }
    return py + mn;
}

void Paddle::Clamp() {
    if (position.y < 0)               position.y = 0;
    if (position.y + height > WIN_HEIGHT) position.y = WIN_HEIGHT - height;
}

void Paddle::SpringSquish(float dt) {
    float a  = -320.0f*(squishY-1.0f) - 14.0f*squishVelY;
    squishVelY += a * dt;
    squishY    += squishVelY * dt;
}

// ─────────────────────────────────────────────────────────────────────────────
// Collision helpers
// ─────────────────────────────────────────────────────────────────────────────
bool BallHitsPaddle(const Ball& b, const Paddle& p) {
    return (b.position.x + b.radius >= p.position.x &&
            b.position.x - b.radius <= p.position.x + p.width &&
            b.position.y + b.radius >= p.position.y &&
            b.position.y - b.radius <= p.position.y + p.height);
}

void ResolvePaddleCollision(Ball& ball, Paddle& paddle, float pushDir) {
    ball.velocity.x  = std::abs(ball.velocity.x) * pushDir;
    float relY       = (ball.position.y - paddle.position.y) / paddle.height;
    ball.velocity.y  = (relY - 0.5f) * 2.0f * 500.0f;
    ball.velocity.x *= (1.0f + BALL_SPEED_INC);

    // --- spin transfer --------------------------------------------------
    // spinning the ball makes the path curve via the Magnus effect in
    // Ball::Update().  We give the paddle's vertical velocity some weight
    // and also factor in where on the paddle we hit (relY).  Users should
    // feel uppercuts/lowercuts by moving the paddle when contacting the
    // ball, and hits near the edges produce a bit more spin as well.
    const float SPIN_FROM_VEL   = 0.5f;   // tweak for difficulty
    const float SPIN_FROM_HIT   = 300.0f; // adds spin when hitting off-centre

    float spin = paddle.velY * SPIN_FROM_VEL;
    spin += (relY - 0.5f) * SPIN_FROM_HIT;
    ball.spin += spin;

    // visual squish & audio feedback
    ball.squish = 1.5f; ball.squishVel = 0.0f;
    paddle.Bounce();
}

float CalcAISpeedFactor(int ps, int as_) {
    float t = (float)(ps - as_) / (float)SCORE_CLAMP;
    t = std::max(-1.0f, std::min(1.0f, t));
    return (t >= 0.0f)
        ? AI_SPEED_BASE + t * (AI_SPEED_MAX - AI_SPEED_BASE)
        : AI_SPEED_BASE + t * (AI_SPEED_BASE - AI_SPEED_MIN);
}