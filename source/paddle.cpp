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

    // Recalculate target periodically (more often at higher difficulty)
    aiRecalcTimer -= dt;
    if (ball.velocity.x > 0) {
        float recalcFreq = 0.05f + (1.0f - aiSpeedFactor / AI_SPEED_MAX) * 0.15f;  // easy = slow updates
        if (aiRecalcTimer <= 0) {
            RecalcTarget(ball);
            aiRecalcTimer = recalcFreq;
        }
    } else {
        aiTargetY = WIN_HEIGHT / 2.0f;
        aiRecalcTimer = 0.0f;
    }

    // Move toward target
    float centre = position.y + height / 2.0f;
    float diff = aiTargetY - centre;
    float spd = PLAYER_SPEED * aiSpeedFactor * dt;
    float move = std::min(std::abs(diff), spd);
    if (diff < -1.0f) position.y -= move;
    if (diff > 1.0f) position.y += move;

    // update velocity after moving
    velY = (position.y - oldY) / dt;

    Clamp();
    SpringSquish(dt);
}

void Paddle::RecalcTarget(const Ball& ball) {
    if (ball.velocity.x <= 0) { aiTargetY = WIN_HEIGHT/2.0f; return; }
    float pred = PredictBallY(ball);
    
    // Difficulty: 0 = easiest, 1 = hardest
    float difficulty = std::max(0.0f, std::min(1.0f,
        (aiSpeedFactor - AI_SPEED_MIN) / (AI_SPEED_MAX - AI_SPEED_MIN)));
    
    // At lowest difficulty: huge error, bad predictions
    // At highest difficulty: tiny error, near-perfect hit
    // Use cubic scaling so low difficulty is VERY dumb
    float errorScale = (1.0f - difficulty) * (1.0f - difficulty) * (1.0f - difficulty);
    
    // Base error from ball speed + difficulty
    float maxErr = 120.0f + (ball.GetSpeed() - BALL_SPEED_INITIAL) * 0.03f;
    float actualErr = maxErr * errorScale;
    
    // At low difficulty, also add some"reaction delay" uncertainty
    if (difficulty < 0.7f) {
        actualErr += (0.7f - difficulty) * 80.0f;
    }
    
    // Random error in range
    aiTargetY = pred + (GetRandomValue(-100, 100) / 100.0f) * actualErr;
    
    // Clamp to keep paddle centered vertically on the hit
    aiTargetY = std::max(height / 2.0f, std::min(WIN_HEIGHT - height / 2.0f, aiTargetY));
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
    if (t < 0) return ball.position.y;

    // Predict Y accounting for spin curvature (magnus effect)
    // Spin creates perpendicular acceleration that curves the ball's path
    float spd = ball.GetSpeed();
    float spinCurve = 0.0f;
    
    if (spd > 0.001f && std::abs(ball.spin) > 0.1f) {
        // Rough approximation: spin causes lateral acceleration
        // magnitude depends on spin, speed, and time to impact
        float spinMag = std::abs(ball.spin) / spd;
        spinCurve = ball.spin * t * t * 0.3f;  // quadratic curve accumulation
    }

    // Base prediction with spin curve
    float py = ball.position.y + ball.velocity.y * t + spinCurve;
    
    // Wall bounces mirror the trajectory
    float mn = ball.radius, mx = WIN_HEIGHT - ball.radius, rng = mx - mn;
    py -= mn;
    if (rng > 0) {
        py = std::fmod(std::abs(py), rng * 2.0f);
        if (py > rng) py = rng * 2.0f - py;
    }
    return py + mn;
}

void Paddle::Clamp() {
    // Always keep paddle from going too far up
    if (position.y < 0) position.y = 0;
    
    // Only clamp bottom if paddle fits on screen
    if (height <= WIN_HEIGHT && position.y + height > WIN_HEIGHT) {
        position.y = WIN_HEIGHT - height;
    }
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

// Swept collision: checks if ball's path from prevPosition to position intersects paddle
bool BallHitsPaddleSwept(const Ball& b, const Paddle& p) {
    // First, do a simple AABB check at current position
    if (BallHitsPaddle(b, p)) return true;
    
    // Check if ball's trajectory crossed the paddle's X boundary
    float prevX = b.prevPosition.x;
    float currX = b.position.x;
    float prevY = b.prevPosition.y;
    float currY = b.position.y;
    
    // Determine which X boundary we need to check (left or right of paddle)
    float paddleEdgeX;
    bool isLeftSide = (prevX - b.radius > p.position.x + p.width); // was to the right, moving left
    bool isRightSide = (prevX + b.radius < p.position.x);          // was to the left, moving right
    
    if (!isLeftSide && !isRightSide) return false;  // wasn't moving toward paddle
    
    paddleEdgeX = isLeftSide ? (p.position.x + p.width) : p.position.x;
    
    // Check if ball crossed this boundary during this frame
    float minX = std::min(prevX, currX);
    float maxX = std::max(prevX, currX);
    
    // Expand for ball radius
    minX -= b.radius;
    maxX += b.radius;
    
    if (minX > paddleEdgeX || maxX < paddleEdgeX) return false;  // didn't cross boundary
    
    // Calculate where on the paddle's Y axis the ball was when it hit the X boundary
    float t = (prevX != currX) ? (paddleEdgeX - prevX) / (currX - prevX) : 0.5f;
    t = std::max(0.0f, std::min(1.0f, t));  // clamp to [0, 1]
    
    float hitY = prevY + (currY - prevY) * t;
    
    // Check if hit Y is within paddle's Y bounds (with ball radius margin)
    if (hitY + b.radius >= p.position.y && hitY - b.radius <= p.position.y + p.height) {
        return true;
    }
    
    return false;
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
    const float SPIN_FROM_VEL   = 0.3f;   // reduced to avoid aggressive curves
    const float SPIN_FROM_HIT   = 150.0f; // reduced edge spin

    // negate velY so upward motion creates backspin (curves forward, not back)
    float spin = -paddle.velY * SPIN_FROM_VEL;
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