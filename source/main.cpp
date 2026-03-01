#include <iostream>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include "raylib.h"
#include "raymath.h"

// ─────────────────────────────────────────────────────────────────────────────
// Constants
// ─────────────────────────────────────────────────────────────────────────────
constexpr int   winWidth           = 800;
constexpr int   winHeight          = 650;
constexpr float BALL_SPEED_INITIAL = 280.0f;  // pixels / second  (dt-based)
constexpr float BALL_SPEED_INC     = 0.12f;   // multiplier per paddle hit
constexpr float PLAYER_SPEED       = 380.0f;  // pixels / second

// AI difficulty
constexpr float AI_SPEED_MIN  = 0.50f;
constexpr float AI_SPEED_MAX  = 2.8f;
constexpr float AI_SPEED_BASE = 1.0f;
constexpr int   SCORE_CLAMP   = 5;

// Economy
constexpr float MULTIPLIER_PER_HIT = 1.15f;
constexpr float BASE_ROUND_REWARD  = 10.0f;
constexpr float MONEY_FLOOR        = -500.0f;

// Palette — cartoony, saturated
constexpr Color COL_BG      = {  18,  20,  40, 255 };
constexpr Color COL_PLAYER  = {  80, 200, 255, 255 };
constexpr Color COL_AI      = { 255,  90,  90, 255 };
constexpr Color COL_BALL    = { 255, 240, 100, 255 };
constexpr Color COL_OUTLINE = {  10,  10,  20, 255 };
constexpr Color COL_GOLD    = { 255, 215,  50, 255 };
constexpr Color COL_GREEN   = {  80, 230, 120, 255 };
constexpr Color COL_RED     = { 255,  80,  80, 255 };

// ─────────────────────────────────────────────────────────────────────────────
// Upgrades
// ─────────────────────────────────────────────────────────────────────────────
enum UpgradeID {
    UPG_PLAYER_SPEED,
    UPG_BALL_SLOW,
    UPG_SHIELD,
    UPG_MULTIPLIER,
    UPG_COUNT
};

struct UpgradeDef {
    const char* name;
    const char* desc;
    float       baseCost;
    float       costScale;
    int         maxLevel;
    Color       accent;
};

static const UpgradeDef UPGRADES[UPG_COUNT] = {
    { "TURBO LEGS",  "+15% player speed",       25.0f, 1.7f, 5, { 130, 255, 130, 255 } },
    { "BALL CHILL",  "-8% ball speed cap",      35.0f, 2.0f, 4, { 200, 150, 255, 255 } },
    { "SHIELD",      "1 free miss per round",   80.0f, 5.0f, 3, { 255, 200,  50, 255 } },
    { "COMBO BOOST", "+0.05 mult bonus/hit",    20.0f, 2.4f, 5, { 255, 140,  60, 255 } },
};

float UpgradeCost(UpgradeID id, int currentLevel) {
    float c = UPGRADES[id].baseCost;
    for (int i = 0; i < currentLevel; i++) c *= UPGRADES[id].costScale;
    return c;
}

// ─────────────────────────────────────────────────────────────────────────────
// AI speed factor
// ─────────────────────────────────────────────────────────────────────────────
float CalcAISpeedFactor(int ps, int as_) {
    float t = (float)(ps - as_) / (float)SCORE_CLAMP;
    t = std::max(-1.0f, std::min(1.0f, t));
    return (t >= 0.0f)
        ? AI_SPEED_BASE + t * (AI_SPEED_MAX - AI_SPEED_BASE)
        : AI_SPEED_BASE + t * (AI_SPEED_BASE - AI_SPEED_MIN);
}

// ─────────────────────────────────────────────────────────────────────────────
// Particles
// ─────────────────────────────────────────────────────────────────────────────
struct Particle {
    Vector2 pos, vel;
    float   life, maxLife, radius, rotation, rotSpeed;
    Color   col;
    bool    square;
};
static std::vector<Particle> gParticles;

void SpawnHitParticles(Vector2 pos, Color col, int count = 18) {
    for (int i = 0; i < count; i++) {
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float spd   = (float)GetRandomValue(60, 280);
        float life  = GetRandomValue(25, 65) / 100.0f;
        Color c = col; c.a = 255;
        gParticles.push_back({ pos,
            { cosf(angle)*spd, sinf(angle)*spd },
            life, life, (float)GetRandomValue(3,9),
            (float)GetRandomValue(0,360),
            (float)GetRandomValue(-400,400),
            c, (bool)GetRandomValue(0,1) });
    }
}

void UpdateParticles(float dt) {
    for (auto& p : gParticles) {
        p.pos.x   += p.vel.x * dt;
        p.pos.y   += p.vel.y * dt;
        p.vel.y   += 320.0f * dt;
        p.rotation += p.rotSpeed * dt;
        p.life     -= dt;
        p.radius   *= 0.985f;
    }
    gParticles.erase(std::remove_if(gParticles.begin(), gParticles.end(),
        [](const Particle& p){ return p.life <= 0.0f; }), gParticles.end());
}

void DrawParticles() {
    for (const auto& p : gParticles) {
        Color c = p.col;
        c.a = (unsigned char)((p.life / p.maxLife) * 255);
        if (p.square) {
            DrawRectanglePro({ p.pos.x - p.radius, p.pos.y - p.radius, p.radius*2, p.radius*2 },
                             { p.radius, p.radius }, p.rotation, c);
        } else {
            DrawCircleV(p.pos, p.radius, c);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Floating text
// ─────────────────────────────────────────────────────────────────────────────
struct FloatText { Vector2 pos; std::string text; Color col; float life, maxLife; int fs; };
static std::vector<FloatText> gFloatTexts;

void SpawnFloatText(Vector2 pos, const char* text, Color col, int fs = 24) {
    gFloatTexts.push_back({ pos, std::string(text), col, 1.4f, 1.4f, fs });
}

void UpdateFloatTexts(float dt) {
    for (auto& f : gFloatTexts) { f.pos.y -= 55.0f * dt; f.life -= dt; }
    gFloatTexts.erase(std::remove_if(gFloatTexts.begin(), gFloatTexts.end(),
        [](const FloatText& f){ return f.life <= 0.0f; }), gFloatTexts.end());
}

void DrawFloatTexts() {
    for (const auto& f : gFloatTexts) {
        float t = f.life / f.maxLife;
        float a = (t < 0.2f) ? (t / 0.2f) : ((t > 0.85f) ? ((1.0f-t)/0.15f*0.6f+0.4f) : 1.0f);
        Color c = f.col; c.a = (unsigned char)(a * 255);
        int w = MeasureText(f.text.c_str(), f.fs);
        for (int ox=-2;ox<=2;ox++) for (int oy=-2;oy<=2;oy++) {
            if (ox==0&&oy==0) continue;
            Color oc = COL_OUTLINE; oc.a = c.a;
            DrawText(f.text.c_str(), (int)(f.pos.x - w/2)+ox, (int)f.pos.y+oy, f.fs, oc);
        }
        DrawText(f.text.c_str(), (int)(f.pos.x - w/2), (int)f.pos.y, f.fs, c);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Screen shake
// ─────────────────────────────────────────────────────────────────────────────
static float gShakeTimer = 0.0f, gShakeStrength = 0.0f;

void TriggerShake(float strength, float dur) { gShakeStrength = strength; gShakeTimer = dur; }

Vector2 ShakeOffset(float dt) {
    if (gShakeTimer <= 0.0f) return {0,0};
    gShakeTimer -= dt;
    float s = gShakeStrength * std::min(gShakeTimer / 0.12f, 1.0f);
    return { (float)GetRandomValue(-1,1)*s, (float)GetRandomValue(-1,1)*s };
}

// ─────────────────────────────────────────────────────────────────────────────
// Cartoony draw helpers
// ─────────────────────────────────────────────────────────────────────────────
void DrawOutlinedRect(Rectangle r, float roundness, Color fill, Color outline, float thick=3.0f) {
    Rectangle o = { r.x-thick, r.y-thick, r.width+thick*2, r.height+thick*2 };
    DrawRectangleRounded(o, roundness, 8, outline);
    DrawRectangleRounded(r, roundness, 8, fill);
}

void DrawOutlinedCircle(Vector2 pos, float r, Color fill, Color outline, float thick=2.5f) {
    DrawCircleV(pos, r+thick, outline);
    DrawCircleV(pos, r, fill);
}

void DrawOutlinedText(const char* text, int x, int y, int fs, Color col, int ow=2) {
    for (int ox=-ow;ox<=ow;ox++) for (int oy=-ow;oy<=ow;oy++) {
        if (ox==0&&oy==0) continue;
        DrawText(text, x+ox, y+oy, fs, COL_OUTLINE);
    }
    DrawText(text, x, y, fs, col);
}

// ─────────────────────────────────────────────────────────────────────────────
// Ball
// ─────────────────────────────────────────────────────────────────────────────
class Ball {
public:
    Vector2 position, velocity;
    float   radius = 12.0f;
    float   squish = 1.0f, squishVel = 0.0f;

    Ball() { position = { winWidth/2.0f, winHeight/2.0f }; velocity = {0,0}; }

    void Reset(int dir) {
        position = { winWidth/2.0f, winHeight/2.0f };
        velocity = { BALL_SPEED_INITIAL * dir,
                     BALL_SPEED_INITIAL * (GetRandomValue(0,1) ? 1.0f : -1.0f) };
        squish = 1.0f; squishVel = 0.0f;
    }

    void CapSpeed(float maxSpd) {
        float spd = GetSpeed();
        if (spd > maxSpd) { velocity.x = velocity.x/spd*maxSpd; velocity.y = velocity.y/spd*maxSpd; }
    }

    void Update(float dt) {
        // Spring squish
        float a = -320.0f*(squish-1.0f) - 12.0f*squishVel;
        squishVel += a*dt; squish += squishVel*dt;

        position.x += velocity.x * dt;
        position.y += velocity.y * dt;

        if (position.y - radius <= 0) {
            position.y = radius; velocity.y = std::abs(velocity.y);
            SpawnHitParticles({position.x, 0}, {180,220,255,255}, 8);
        }
        if (position.y + radius >= winHeight) {
            position.y = winHeight-radius; velocity.y = -std::abs(velocity.y);
            SpawnHitParticles({position.x,(float)winHeight}, {180,220,255,255}, 8);
        }
    }

    float GetSpeed() const { return sqrtf(velocity.x*velocity.x + velocity.y*velocity.y); }

    void Draw() const {
        float rx = radius * std::max(0.3f, squish);
        float ry = radius / std::max(0.3f, squish);
        DrawEllipse((int)position.x, (int)position.y, rx+3, ry+3, COL_OUTLINE);
        DrawEllipse((int)position.x, (int)position.y, rx, ry, COL_BALL);
        // Shine
        DrawEllipse((int)(position.x - rx*0.2f), (int)(position.y - ry*0.25f),
                    rx*0.35f, ry*0.25f, {255,255,220,120});
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Paddle
// ─────────────────────────────────────────────────────────────────────────────
class Paddle {
public:
    Vector2 position;
    float   width=16, height=100, playerSpeed=PLAYER_SPEED, aiSpeedFactor=AI_SPEED_BASE;
    float   squishY=1.0f, squishVelY=0.0f;

    Paddle(float x, float y) : position({x, y}) {}

    void UpdatePlayer(KeyboardKey up, KeyboardKey dn, float dt) {
        if (IsKeyDown(up))  position.y -= playerSpeed * dt;
        if (IsKeyDown(dn))  position.y += playerSpeed * dt;
        Clamp();
        SpringSquish(dt);
    }

    void UpdateAI(const Ball& ball, float dt) {
        float target = (ball.velocity.x > 0) ? aiTargetY : winHeight/2.0f;
        if (ball.velocity.x <= 0) aiTargetY = winHeight/2.0f;
        float centre = position.y + height/2.0f;
        float diff   = target - centre;
        float spd    = PLAYER_SPEED * aiSpeedFactor * dt;
        float move   = std::min(std::abs(diff), spd);
        if (diff < -1.0f) position.y -= move;
        if (diff >  1.0f) position.y += move;
        Clamp();
        SpringSquish(dt);
    }

    void RecalcTarget(const Ball& ball) {
        if (ball.velocity.x <= 0) { aiTargetY = winHeight/2.0f; return; }
        float pred    = PredictBallY(ball);
        float errRange = 10.0f + (ball.GetSpeed() - BALL_SPEED_INITIAL) * 0.015f;
        errRange = std::min(errRange, height * 0.72f);
        float t = std::max(0.0f, std::min(1.0f, (aiSpeedFactor - AI_SPEED_MIN) / (AI_SPEED_MAX - AI_SPEED_MIN)));
        errRange *= (0.02f + 0.98f*(1.0f - t));
        aiTargetY = pred + (GetRandomValue(-100,100)/100.0f) * errRange;
    }

    void Bounce() { squishY = 0.55f; squishVelY = 0.0f; }

    void Draw(Color col) const {
        float h  = height * std::max(0.3f, squishY);
        float cy = position.y + height/2.0f - h/2.0f;
        DrawOutlinedRect({ position.x, cy, width, h }, 0.5f, col, COL_OUTLINE, 3.0f);
        // Shine strip
        DrawRectangleRounded({ position.x+3, cy+h*0.12f, width-6, h*0.35f },
                             0.5f, 4, Fade(WHITE, 0.2f));
    }

    Rectangle GetRect() const { return { position.x, position.y, width, height }; }

private:
    float aiTargetY = winHeight/2.0f;

    float PredictBallY(const Ball& ball) const {
        float vx = ball.velocity.x;
        if (vx <= 0) return ball.position.y;
        float t  = (position.x - ball.position.x) / vx;
        float py = ball.position.y + ball.velocity.y * t;
        float mn = ball.radius, mx = winHeight - ball.radius, rng = mx - mn;
        py -= mn;
        if (rng > 0) {
            py = std::fmod(std::abs(py), rng*2.0f);
            if (py > rng) py = rng*2.0f - py;
        }
        return py + mn;
    }

    void Clamp() {
        if (position.y < 0) position.y = 0;
        if (position.y + height > winHeight) position.y = winHeight - height;
    }

    void SpringSquish(float dt) {
        float a = -320.0f*(squishY-1.0f) - 14.0f*squishVelY;
        squishVelY += a*dt; squishY += squishVelY*dt;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Collision
// ─────────────────────────────────────────────────────────────────────────────
bool BallHitsPaddle(const Ball& b, const Paddle& p) {
    return (b.position.x+b.radius >= p.position.x &&
            b.position.x-b.radius <= p.position.x+p.width &&
            b.position.y+b.radius >= p.position.y &&
            b.position.y-b.radius <= p.position.y+p.height);
}

void ResolvePaddleCollision(Ball& ball, Paddle& paddle, float pushDir) {
    ball.velocity.x  = std::abs(ball.velocity.x) * pushDir;
    float relY       = (ball.position.y - paddle.position.y) / paddle.height;
    ball.velocity.y  = (relY - 0.5f) * 2.0f * 500.0f;
    ball.velocity.x *= (1.0f + BALL_SPEED_INC);
    ball.squish = 1.5f; ball.squishVel = 0.0f;
    paddle.Bounce();
}

float CalcMultiplier(int hits, float bonus) {
    float m = 1.0f;
    for (int i = 0; i < hits; i++) m *= (MULTIPLIER_PER_HIT + bonus);
    return m;
}

// ─────────────────────────────────────────────────────────────────────────────
// HUD helpers
// ─────────────────────────────────────────────────────────────────────────────
void DrawStatPanel(int x, int y, const char* label, const char* value,
                   Color accent, bool highlight=false) {
    int pw=162, ph=54;
    DrawOutlinedRect({(float)x,(float)y,(float)pw,(float)ph}, 0.35f, {20,22,45,220}, COL_OUTLINE, 2.5f);
    DrawRectangleRounded({(float)x+2,(float)y,(float)(pw-4),4.0f}, 0.5f, 4, accent);
    if (highlight)
        DrawRectangleRounded({(float)(x-1),(float)(y-1),(float)(pw+2),(float)(ph+2)}, 0.35f, 8, Fade(accent,0.15f));
    DrawOutlinedText(label, x+10, y+8, 11, Fade(accent,0.9f), 1);
    DrawOutlinedText(value, x+10, y+24, 20, WHITE, 2);
}

void DrawDifficultyBar(float factor) {
    float t = std::max(0.0f, std::min(1.0f, (factor-AI_SPEED_MIN)/(AI_SPEED_MAX-AI_SPEED_MIN)));
    int bw=130, bh=10, bx=winWidth/2-65, by=80;
    DrawOutlinedRect({(float)bx,(float)by,(float)bw,(float)bh}, 0.5f, {30,32,58,220}, COL_OUTLINE, 2.0f);
    if (bw*t >= 1.0f) {
        Color fill = (t < 0.5f)
            ? Color{(unsigned char)(80+(int)(175*t*2)),220,80,255}
            : Color{255,(unsigned char)(220-(int)(220*(t-0.5f)*2)),80,255};
        DrawRectangleRounded({(float)bx,(float)by,bw*t,(float)bh}, 0.5f, 4, fill);
    }
    int lw = MeasureText("AI DIFFICULTY", 10);
    DrawText("AI DIFFICULTY", winWidth/2-lw/2, by-14, 10, Fade(WHITE,0.45f));
}

// ─────────────────────────────────────────────────────────────────────────────
// Upgrade shop
// ─────────────────────────────────────────────────────────────────────────────
void DrawUpgradeShop(const int upgLevels[UPG_COUNT], float playerMoney, int& hovered) {
    DrawRectangle(0,0,winWidth,winHeight,{0,0,0,175});
    int pw=500, ph=400, px=winWidth/2-250, py=winHeight/2-200;
    DrawOutlinedRect({(float)px,(float)py,(float)pw,(float)ph}, 0.12f, {15,18,38,248}, COL_GOLD, 3.5f);

    int tw = MeasureText("UPGRADE SHOP", 28);
    DrawOutlinedText("UPGRADE SHOP", winWidth/2-tw/2, py+14, 28, COL_GOLD, 2);

    const char* coinStr = TextFormat("Available: %.0f coins", playerMoney);
    int cw = MeasureText(coinStr,15);
    DrawText(coinStr, winWidth/2-cw/2, py+52, 15, COL_GREEN);

    hovered = -1;
    int mx = GetMouseX(), my = GetMouseY();

    for (int i = 0; i < UPG_COUNT; i++) {
        int bx=px+20, by2=py+80+i*58, bw2=pw-40, bh2=50;
        bool maxed   = (upgLevels[i] >= UPGRADES[i].maxLevel);
        float cost   = maxed ? 0 : UpgradeCost((UpgradeID)i, upgLevels[i]);
        bool canBuy  = (!maxed && playerMoney >= cost);
        bool over    = (mx>=bx&&mx<=bx+bw2&&my>=by2&&my<=by2+bh2);
        if (over) hovered = i;

        Color bg   = over && canBuy ? Color{30,40,80,255} : Color{20,24,50,240};
        Color bord = maxed ? COL_GREEN : (canBuy ? UPGRADES[i].accent : Color{60,60,80,255});
        DrawOutlinedRect({(float)bx,(float)by2,(float)bw2,(float)bh2}, 0.25f, bg, bord, 2.5f);
        DrawOutlinedText(UPGRADES[i].name, bx+10, by2+7, 16, maxed ? COL_GREEN : WHITE, 1);
        DrawText(UPGRADES[i].desc, bx+10, by2+28, 11, Fade(WHITE,0.55f));

        // Level pips
        for (int l=0; l<UPGRADES[i].maxLevel; l++) {
            Color dc = (l < upgLevels[i]) ? UPGRADES[i].accent : Color{40,40,70,255};
            DrawOutlinedCircle({(float)(bx+bw2-18-l*16),(float)(by2+14)}, 5.0f, dc, COL_OUTLINE, 1.5f);
        }

        if (maxed)
            DrawText("MAXED", bx+bw2-58, by2+31, 13, COL_GREEN);
        else
            DrawText(TextFormat("%.0f c", cost), bx+bw2-60, by2+31, 13, canBuy ? COL_GOLD : COL_RED);
    }
    int fw = MeasureText("[ Click to buy ]   [ TAB to close ]", 12);
    DrawText("[ Click to buy ]   [ TAB to close ]",
             winWidth/2-fw/2, py+ph-22, 12, Fade(WHITE,0.4f));
}

// ─────────────────────────────────────────────────────────────────────────────
// Main
// ─────────────────────────────────────────────────────────────────────────────
int main() {
    SetConfigFlags(FLAG_VSYNC_HINT);            // VSync — no SetTargetFPS needed
    InitWindow(winWidth, winHeight, "Rick's Pong");
    InitAudioDevice();

    Sound sndPlayer  = LoadSound("../sounds/ball_collision1.wav");
    Sound sndAI      = LoadSound("../sounds/ball_collision2.wav");
    Sound sndCollect = LoadSound("../sounds/collect.wav");

    Music music = LoadMusicStream("../sounds/soundtrack.ogg");
    SetMusicVolume(music, 0.0f);
    PlayMusicStream(music);
    float musicVol = 0.0f;

    Ball   ball;
    Paddle player(50,            winHeight/2.0f - 50);
    Paddle ai    (winWidth - 66, winHeight/2.0f - 50);

    int   playerScore = 0, aiScore = 0;
    float playerMoney = 0.0f;
    int   roundHits   = 0;
    float roundMult   = 1.0f;
    int   shieldHP    = 0;

    int upgLevels[UPG_COUNT] = {};

    auto ApplyUpgrades = [&]() {
        player.playerSpeed = PLAYER_SPEED * (1.0f + upgLevels[UPG_PLAYER_SPEED] * 0.15f);
    };
    ApplyUpgrades();

    auto GetBallMaxSpeed = [&]() {
        return 1400.0f * (1.0f - upgLevels[UPG_BALL_SLOW] * 0.08f);
    };
    auto GetBonusMult = [&]() { return upgLevels[UPG_MULTIPLIER] * 0.05f; };

    enum ResultType { RESULT_NONE, RESULT_WIN, RESULT_LOSE } lastResult = RESULT_NONE;
    float resultTimer = 0.0f, resultDelta = 0.0f;
    constexpr float RESULT_TIME = 2.2f;

    enum State { WAITING, PLAYING, SHOP } state = WAITING;
    float prevVelY = 0.0f;
    int   hoveredUpgrade = -1;

    // Background stars
    struct Star { Vector2 pos; float r, bright; };
    std::vector<Star> stars;
    for (int i = 0; i < 90; i++)
        stars.push_back({{ (float)GetRandomValue(0,winWidth),(float)GetRandomValue(0,winHeight) },
                          GetRandomValue(1,3)*1.0f, GetRandomValue(20,100)/100.0f });

    // Score pop animations
    float playerPop = 1.0f, aiPop = 1.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;   // clamp huge spikes

        // Music fade in
        musicVol = std::min(musicVol + 0.4f*dt, 0.85f);
        SetMusicVolume(music, musicVol);
        UpdateMusicStream(music);

        if (resultTimer > 0) resultTimer -= dt;
        // Decay pop
        if (playerPop > 1.0f) playerPop = std::max(1.0f, playerPop - 3.5f*dt);
        if (aiPop     > 1.0f) aiPop     = std::max(1.0f, aiPop     - 3.5f*dt);

        // Live difficulty update
        ai.aiSpeedFactor = CalcAISpeedFactor(playerScore, aiScore);

        // ── Input / state ─────────────────────────────────────────────────
        if (state == WAITING) {
            if (IsKeyPressed(KEY_SPACE)) {
                shieldHP = upgLevels[UPG_SHIELD];
                ball.Reset(1);
                ai.RecalcTarget(ball);
                prevVelY  = ball.velocity.y;
                roundHits = 0; roundMult = 1.0f;
                state     = PLAYING;
            }
            if (IsKeyPressed(KEY_TAB)) state = SHOP;
        }
        else if (state == SHOP) {
            if (IsKeyPressed(KEY_TAB)) { ApplyUpgrades(); state = WAITING; }
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoveredUpgrade >= 0) {
                int i    = hoveredUpgrade;
                float cost = UpgradeCost((UpgradeID)i, upgLevels[i]);
                if (upgLevels[i] < UPGRADES[i].maxLevel && playerMoney >= cost) {
                    playerMoney -= cost;
                    upgLevels[i]++;
                    ApplyUpgrades();
                    PlaySound(sndCollect);
                    SpawnFloatText({winWidth/2.0f, winHeight/2.0f+90},
                                   TextFormat("Upgraded %s!", UPGRADES[i].name),
                                   UPGRADES[i].accent, 22);
                }
            }
        }
        else if (state == PLAYING) {
            player.UpdatePlayer(KEY_W, KEY_S, dt);
            ai.UpdateAI(ball, dt);
            ball.Update(dt);
            ball.CapSpeed(GetBallMaxSpeed());

            // Recalc AI target on ball Y direction flip
            float cy = ball.velocity.y;
            if (std::abs(cy) > 0.001f && std::abs(prevVelY) > 0.001f
                && ((cy>0) != (prevVelY>0)))
                ai.RecalcTarget(ball);
            prevVelY = cy;

            // Player hit
            if (ball.velocity.x < 0 && BallHitsPaddle(ball, player)) {
                ResolvePaddleCollision(ball, player, +1.0f);
                ball.position.x = player.position.x + player.width + ball.radius + 1.0f;
                ai.RecalcTarget(ball);
                PlaySound(sndPlayer);
                SpawnHitParticles(ball.position, COL_PLAYER, 20);
                TriggerShake(5.0f, 0.10f);
                roundHits++;
                roundMult = CalcMultiplier(roundHits, GetBonusMult());
                if (roundMult > 1.01f)
                    SpawnFloatText({player.position.x+55, player.position.y},
                                   TextFormat("x%.2f", roundMult),
                                   roundMult > 2.0f ? COL_GOLD : Fade(WHITE,0.9f), 18);
            }

            // AI hit
            if (ball.velocity.x > 0 && BallHitsPaddle(ball, ai)) {
                ResolvePaddleCollision(ball, ai, -1.0f);
                ball.position.x = ai.position.x - ball.radius - 1.0f;
                PlaySound(sndAI);
                SpawnHitParticles(ball.position, COL_AI, 20);
                TriggerShake(4.0f, 0.08f);
                roundHits++;
                roundMult = CalcMultiplier(roundHits, GetBonusMult());
            }

            // Ball exits left → AI scores (or shield absorbs)
            if (ball.position.x + ball.radius < 0) {
                if (shieldHP > 0) {
                    shieldHP--;
                    ball.Reset(1);
                    ai.RecalcTarget(ball);
                    prevVelY = ball.velocity.y;
                    roundHits = 0; roundMult = 1.0f;
                    SpawnFloatText({winWidth/2.0f, winHeight/2.0f-50},
                                   "SHIELD!", {255,220,50,255}, 36);
                    SpawnHitParticles({winWidth/2.0f, winHeight/2.0f}, COL_GOLD, 35);
                    TriggerShake(6.0f, 0.12f);
                } else {
                    aiScore++;
                    aiPop      = 1.5f;
                    state      = WAITING;
                    float loss = BASE_ROUND_REWARD * roundMult;
                    float newM = std::max(MONEY_FLOOR, playerMoney - loss);
                    resultDelta = playerMoney - newM;
                    playerMoney = newM;
                    lastResult  = RESULT_LOSE;
                    resultTimer = RESULT_TIME;
                    SpawnHitParticles({0, winHeight/2.0f}, COL_AI, 40);
                    TriggerShake(12.0f, 0.20f);
                }
            }

            // Ball exits right → Player scores
            if (ball.position.x - ball.radius > winWidth) {
                playerScore++;
                playerPop    = 1.5f;
                state        = WAITING;
                float win    = BASE_ROUND_REWARD * roundMult;
                playerMoney += win;
                lastResult   = RESULT_WIN;
                resultDelta  = win;
                resultTimer  = RESULT_TIME;
                PlaySound(sndCollect);
                SpawnHitParticles({(float)winWidth, winHeight/2.0f}, COL_PLAYER, 40);
                TriggerShake(8.0f, 0.15f);
            }
        }

        UpdateParticles(dt);
        UpdateFloatTexts(dt);
        Vector2 shake = ShakeOffset(dt);

        // ── Render ────────────────────────────────────────────────────────
        BeginDrawing();
        ClearBackground(COL_BG);

        // Stars
        for (const auto& s : stars)
            DrawCircleV(s.pos, s.r, Fade(WHITE, s.bright * 0.35f));

        BeginMode2D(Camera2D{shake,{0,0},0,1});

        // Centre dashes
        for (int y=0; y<winHeight; y+=28)
            DrawRectangleRounded({winWidth/2.0f-3,(float)y,6,16}, 0.5f, 4, Fade(WHITE,0.10f));

        // Scores with pop
        auto DrawScore = [&](int score, float pop, int cx, int topY) {
            const char* s = TextFormat("%d", score);
            int fs = (int)(48 * pop);
            int w  = MeasureText(s, fs);
            DrawOutlinedText(s, cx-w/2, topY, fs, WHITE, 2);
        };
        DrawScore(playerScore, playerPop, winWidth/2-45, 16);
        DrawScore(aiScore,     aiPop,     winWidth/2+45, 16);

        DrawDifficultyBar(ai.aiSpeedFactor);

        // Game objects
        player.Draw(COL_PLAYER);
        ai.Draw(COL_AI);
        if (state == PLAYING) ball.Draw();

        DrawParticles();
        DrawFloatTexts();

        // Bottom HUD
        float mult   = (state==PLAYING) ? roundMult : 1.0f;
        bool  hotM   = (mult > 1.0f);
        DrawStatPanel(14, winHeight-72, "MULTIPLIER",
                      TextFormat("x%.2f", mult),
                      hotM ? COL_GOLD : Color{100,160,255,255}, hotM);

        bool neg = (playerMoney < 0);
        DrawStatPanel(winWidth-176, winHeight-72, "COINS",
                      TextFormat("%.0f", playerMoney), neg ? COL_RED : COL_GREEN, neg);

        // Shield pips
        for (int s=0; s<shieldHP; s++)
            DrawOutlinedCircle({(float)(winWidth/2 - (shieldHP-1)*13 + s*26),
                                (float)(winHeight-18)}, 7.0f, COL_GOLD, COL_OUTLINE, 2.0f);

        // Stake label
        if (state == PLAYING) {
            const char* sl = TextFormat("STAKE: +%.0f", BASE_ROUND_REWARD * roundMult);
            DrawText(sl, winWidth/2 - MeasureText(sl,13)/2, winHeight-48, 13,
                     Fade({200,200,255,255}, 0.65f));
        }

        // Result flash
        if (resultTimer > 0) {
            float a = std::min(resultTimer/0.3f,1.0f) *
                      std::min((RESULT_TIME-resultTimer)/0.3f+0.2f,1.0f);
            a = std::min(a, 1.0f);
            bool won = (lastResult == RESULT_WIN);
            Color rc = won ? COL_GREEN : COL_RED;
            const char* big = won ? "YOU SCORED!" : "AI SCORED!";
            const char* sub = won ? TextFormat("+%.0f COINS", resultDelta)
                                  : TextFormat("-%.0f COINS", resultDelta);
            int bw2 = MeasureText(big,38), sw2 = MeasureText(sub,22);
            DrawRectangleRounded({(float)(winWidth/2-175),(float)(winHeight/2-55),350,95},
                                 0.3f,8,Fade({8,8,22,235},a));
            DrawRectangleRoundedLines({(float)(winWidth/2-175),(float)(winHeight/2-55),350,95},
                                      0.3f,8,Fade(rc,a));
            DrawOutlinedText(big, winWidth/2-bw2/2, winHeight/2-40, 38, Fade(WHITE,a), 2);
            DrawText(sub, winWidth/2-sw2/2, winHeight/2+8, 22, Fade(rc,a));
        }

        // Waiting prompt
        if (state == WAITING) {
            float pulse = 0.6f + 0.4f*sinf((float)GetTime()*4.0f);
            const char* sp  = "PRESS SPACE TO SERVE";
            const char* tl  = "[ TAB ] UPGRADE SHOP";
            DrawText(sp,  winWidth/2-MeasureText(sp, 20)/2,  winHeight/2+30, 20, Fade(WHITE,pulse));
            DrawText(tl,  winWidth/2-MeasureText(tl, 14)/2,  winHeight/2+60, 14, Fade(COL_GOLD,pulse*0.8f));
        }

        // Shop overlay
        if (state == SHOP)
            DrawUpgradeShop(upgLevels, playerMoney, hoveredUpgrade);

        EndMode2D();
        EndDrawing();
    }

    UnloadSound(sndPlayer);
    UnloadSound(sndAI);
    UnloadSound(sndCollect);
    StopMusicStream(music);
    UnloadMusicStream(music);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}