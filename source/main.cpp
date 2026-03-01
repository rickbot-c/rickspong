#include <cmath>
#include <vector>
#include <algorithm>

#include "raylib.h"
#include "raymath.h"

#include "Constants.h"
#include "Upgrades.h"
#include "Player.h"
#include "Ball.h"
#include "Paddle.h"
#include "Particles.h"
#include "HUD.h"

// ─────────────────────────────────────────────────────────────────────────────
// Game states
// ─────────────────────────────────────────────────────────────────────────────
enum State    { WAITING, PLAYING, SHOP };
enum ResultType { RESULT_NONE, RESULT_WIN, RESULT_LOSE };

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetTargetFPS(0);
    SetExitKey(KEY_NULL); // disable default ESC exit
    InitWindow(WIN_WIDTH, WIN_HEIGHT, "Rick's Pong");
    InitAudioDevice();

    Sound sndPlayer  = LoadSound("../sounds/ball_collision1.wav");
    Sound sndAI      = LoadSound("../sounds/ball_collision2.wav");
    Sound sndCollect = LoadSound("../sounds/collect.wav");
    Sound sndLevelUp = LoadSound("../sounds/collect.wav");   // swap for a dedicated clip later

    Image icon = LoadImage("../images/icon.png");
    SetWindowIcon(icon);

    Music music = LoadMusicStream("../sounds/soundtrack.ogg");
    SetMusicVolume(music, 0.0f);
    PlayMusicStream(music);
    float musicVol = 0.0f;        // current volume, smoothly ramps to max

    // ── Game objects ─────────────────────────────────────────────────────────
    Ball   ball;
    Paddle playerPaddle(50,              WIN_HEIGHT/2.0f - 50);
    Paddle aiPaddle    (WIN_WIDTH - 66,  WIN_HEIGHT/2.0f - 50);
    Player player;

    // ── Match state ──────────────────────────────────────────────────────────
    int   playerScore = 0, aiScore = 0;
    int   roundHits   = 0;
    float roundMult   = 1.0f;

    State      state      = WAITING;
    ResultType lastResult = RESULT_NONE;
    float resultTimer = 0.0f, resultDelta = 0.0f;
    constexpr float RESULT_TIME = 2.2f;

    float prevVelY      = 0.0f;
    int   hoveredUpgrade = -1;

    // Score pop animations
    float playerPop = 1.0f, aiPop = 1.0f;

    // Background stars
    struct Star { Vector2 pos; float r, bright; };
    std::vector<Star> stars;
    for (int i = 0; i < 90; i++)
        stars.push_back({
            { (float)GetRandomValue(0,WIN_WIDTH), (float)GetRandomValue(0,WIN_HEIGHT) },
            GetRandomValue(1,3)*1.0f,
            GetRandomValue(20,100)/100.0f });

    // ── Lambdas ───────────────────────────────────────────────────────────────
    auto syncPaddleSpeed = [&]() {
        playerPaddle.playerSpeed = PLAYER_SPEED * player.GetPlayerSpeedMult();
    };
    syncPaddleSpeed();

    // ─────────────────────────────────────────────────────────────────────────
    // Game loop
    // ─────────────────────────────────────────────────────────────────────────
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

        // Music fade in
        musicVol = std::min(musicVol + MUSIC_FADE_IN_SPEED * dt,
                              MUSIC_MAX_VOLUME);
        SetMusicVolume(music, musicVol);
        UpdateMusicStream(music);

        if (resultTimer > 0) resultTimer -= dt;
        if (playerPop > 1.0f) playerPop = std::max(1.0f, playerPop - 3.5f*dt);
        if (aiPop     > 1.0f) aiPop     = std::max(1.0f, aiPop     - 3.5f*dt);

        // Live difficulty
        aiPaddle.aiSpeedFactor = CalcAISpeedFactor(playerScore, aiScore);

        // ── Input / state ─────────────────────────────────────────────────
        if (state == WAITING) {
            if (IsKeyPressed(KEY_SPACE)) {
                player.PrepareShields();
                ball.Reset(1);
                aiPaddle.RecalcTarget(ball);
                prevVelY  = ball.velocity.y;
                roundHits = 0;
                roundMult = 1.0f;
                state     = PLAYING;
            }
            if (IsKeyPressed(KEY_TAB)) state = SHOP;
        }
        else if (state == SHOP) {
            if (IsKeyPressed(KEY_TAB)) { syncPaddleSpeed(); state = WAITING; }
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hoveredUpgrade >= 0) {
                UpgradeID id = (UpgradeID)hoveredUpgrade;
                if (player.BuyUpgrade(id)) {
                    syncPaddleSpeed();
                    PlaySound(sndCollect);
                    SpawnFloatText({ WIN_WIDTH/2.0f, WIN_HEIGHT/2.0f+90 },
                                   TextFormat("Upgraded %s!", UPGRADES[id].name),
                                   UPGRADES[id].accent, 22);
                }
            }
        }
        else if (state == PLAYING) {
            playerPaddle.UpdatePlayer(KEY_W, KEY_S, dt);
            aiPaddle.UpdateAI(ball, dt);
            ball.Update(dt);
            ball.CapSpeed(player.GetBallMaxSpeed());

            // Recalc AI target on ball Y flip
            float cy = ball.velocity.y;
            if (std::abs(cy) > 0.001f && std::abs(prevVelY) > 0.001f
                && ((cy > 0) != (prevVelY > 0)))
                aiPaddle.RecalcTarget(ball);
            prevVelY = cy;

            // ── Player paddle hit ─────────────────────────────────────────
            if (ball.velocity.x < 0 && BallHitsPaddle(ball, playerPaddle)) {
                ResolvePaddleCollision(ball, playerPaddle, +1.0f);
                ball.position.x = playerPaddle.position.x + playerPaddle.width + ball.radius + 1.0f;
                aiPaddle.RecalcTarget(ball);
                PlaySound(sndPlayer);
                SpawnHitParticles(ball.position, COL_PLAYER, 20);
                TriggerShake(5.0f, 0.10f);
                roundHits++;
                roundMult = CalcMultiplier(roundHits, player.GetBonusMult());
                if (roundMult > 1.01f)
                    SpawnFloatText({ playerPaddle.position.x+55, playerPaddle.position.y },
                                   TextFormat("x%.2f", roundMult),
                                   roundMult > 2.0f ? COL_GOLD : Fade(WHITE, 0.9f), 18);
            }

            // ── AI paddle hit ─────────────────────────────────────────────
            if (ball.velocity.x > 0 && BallHitsPaddle(ball, aiPaddle)) {
                ResolvePaddleCollision(ball, aiPaddle, -1.0f);
                ball.position.x = aiPaddle.position.x - ball.radius - 1.0f;
                PlaySound(sndAI);
                SpawnHitParticles(ball.position, COL_AI, 20);
                TriggerShake(4.0f, 0.08f);
                roundHits++;
                roundMult = CalcMultiplier(roundHits, player.GetBonusMult());
            }

            // ── Ball exits left → AI scores (or shield absorbs) ───────────
            if (ball.position.x + ball.radius < 0) {
                if (player.ConsumeShield()) {
                    ball.Reset(1);
                    aiPaddle.RecalcTarget(ball);
                    prevVelY  = ball.velocity.y;
                    roundHits = 0;
                    roundMult = 1.0f;
                    SpawnFloatText({ WIN_WIDTH/2.0f, WIN_HEIGHT/2.0f-50 },
                                   "SHIELD!", { 255,220,50,255 }, 36);
                    SpawnHitParticles({ WIN_WIDTH/2.0f, WIN_HEIGHT/2.0f }, COL_GOLD, 35);
                    TriggerShake(6.0f, 0.12f);
                } else {
                    aiScore++;
                    aiPop = 1.5f;
                    state = WAITING;
                    float loss = std::max(BASE_ROUND_LOSS, BASE_ROUND_REWARD * roundMult * 0.8f);
                    resultDelta = loss;
                    player.LoseMoney(loss);
                    lastResult  = RESULT_LOSE;
                    resultTimer = RESULT_TIME;
                    SpawnHitParticles({ 0, WIN_HEIGHT/2.0f }, COL_AI, 40);
                    TriggerShake(12.0f, 0.20f);
                }
            }

            // ── Ball exits right → Player scores ─────────────────────────
            if (ball.position.x - ball.radius > WIN_WIDTH) {
                playerScore++;
                playerPop   = 1.5f;
                state       = WAITING;
                float win   = BASE_ROUND_REWARD * roundMult;
                player.AddMoney(win);
                resultDelta = win;
                lastResult  = RESULT_WIN;
                resultTimer = RESULT_TIME;
                PlaySound(sndCollect);
                SpawnHitParticles({ (float)WIN_WIDTH, WIN_HEIGHT/2.0f }, COL_PLAYER, 40);
                TriggerShake(8.0f, 0.15f);

                // ── Award XP ─────────────────────────────────────────────
                float xpGain = XP_WIN_BASE + roundHits * XP_RALLY_BONUS;
                player.AddXP(xpGain);
                SpawnFloatText({ WIN_WIDTH/2.0f, WIN_HEIGHT/2.0f - 80 },
                               TextFormat("+%.0f XP", xpGain), COL_XP, 20);

                if (player.levelledUp) {
                    PlaySound(sndLevelUp);
                    SpawnFloatText({ WIN_WIDTH/2.0f, WIN_HEIGHT/2.0f - 120 },
                                   TextFormat("LEVEL UP!  LVL %d", player.level),
                                   COL_GOLD, 32);
                    SpawnHitParticles({ WIN_WIDTH/2.0f, WIN_HEIGHT/2.0f }, COL_GOLD, 50);
                    TriggerShake(10.0f, 0.18f);
                }
            }
        }

        UpdateParticles(dt);
        UpdateFloatTexts(dt);
        Vector2 shake = ShakeOffset(dt);

        // ── Render ────────────────────────────────────────────────────────
        BeginDrawing();
        ClearBackground(COL_BG);

        if (IsKeyDown(KEY_LEFT_CONTROL))
            DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 14, Fade(WHITE, 0.4f));

        // Stars
        for (const auto& s : stars)
            DrawCircleV(s.pos, s.r, Fade(WHITE, s.bright * 0.35f));

        BeginMode2D(Camera2D{ shake, {0,0}, 0, 1 });

        // Centre dashes
        for (int y = 0; y < WIN_HEIGHT; y += 28)
            DrawRectangleRounded({ WIN_WIDTH/2.0f-3, (float)y, 6, 16 },
                                 0.5f, 4, Fade(WHITE, 0.10f));

        // Scores
        auto DrawScore = [&](int score, float pop, int cx, int topY) {
            const char* s = TextFormat("%d", score);
            int fs = (int)(48 * pop);
            int w  = MeasureText(s, fs);
            DrawOutlinedText(s, cx-w/2, topY, fs, WHITE, 2);
        };
        DrawScore(playerScore, playerPop, WIN_WIDTH/2-45, 16);
        DrawScore(aiScore,     aiPop,     WIN_WIDTH/2+45, 16);

        DrawDifficultyBar(aiPaddle.aiSpeedFactor);

        playerPaddle.Draw(COL_PLAYER);
        aiPaddle.Draw(COL_AI);
        if (state == PLAYING) ball.Draw();

        DrawParticles();
        DrawFloatTexts();

        // ── Bottom HUD ────────────────────────────────────────────────────
        DrawXPBar(player);

        float mult = (state == PLAYING) ? roundMult : 1.0f;
        bool  hotM = (mult > 1.0f);
        DrawStatPanel(14, WIN_HEIGHT-72, "MULTIPLIER",
                      TextFormat("x%.2f", mult),
                      hotM ? COL_GOLD : Color{100,160,255,255}, hotM);

        bool neg = (player.money < 0);
        DrawStatPanel(WIN_WIDTH-176, WIN_HEIGHT-72, "COINS",
                      TextFormat("%.0f", player.money),
                      neg ? COL_RED : COL_GREEN, neg);

        DrawShieldPips(player.shieldHP, player.upgLevels[UPG_SHIELD],
                       player.money, state == WAITING ? 0 : 1);

        if (state == PLAYING) {
            const char* sl = TextFormat("STAKE: +%.0f", BASE_ROUND_REWARD * roundMult);
            DrawText(sl, WIN_WIDTH/2 - MeasureText(sl,13)/2, WIN_HEIGHT-48, 13,
                     Fade({200,200,255,255}, 0.65f));
        }

        // ── Result flash ──────────────────────────────────────────────────
        if (resultTimer > 0) {
            float a = std::min(resultTimer/0.3f, 1.0f) *
                      std::min((RESULT_TIME-resultTimer)/0.3f+0.2f, 1.0f);
            a = std::min(a, 1.0f);
            bool  won = (lastResult == RESULT_WIN);
            Color rc  = won ? COL_GREEN : COL_RED;
            const char* big = won ? "YOU SCORED!" : "AI SCORED!";
            const char* sub = won ? TextFormat("+%.0f COINS", resultDelta)
                                  : TextFormat("-%.0f COINS", resultDelta);
            int bw2 = MeasureText(big, 38), sw2 = MeasureText(sub, 22);
            DrawRectangleRounded({(float)(WIN_WIDTH/2-175),(float)(WIN_HEIGHT/2-55),350,95},
                                 0.3f, 8, Fade({8,8,22,235}, a));
            DrawRectangleRoundedLines({(float)(WIN_WIDTH/2-175),(float)(WIN_HEIGHT/2-55),350,95},
                                      0.3f, 8, Fade(rc, a));
            DrawOutlinedText(big, WIN_WIDTH/2-bw2/2, WIN_HEIGHT/2-40, 38, Fade(WHITE, a), 2);
            DrawText(sub, WIN_WIDTH/2-sw2/2, WIN_HEIGHT/2+8, 22, Fade(rc, a));
        }

        // ── Waiting prompt ────────────────────────────────────────────────
        if (state == WAITING) {
            float pulse = 0.6f + 0.4f*sinf((float)GetTime()*4.0f);
            const char* sp = "PRESS SPACE TO SERVE";
            const char* tl = "[ TAB ] UPGRADE SHOP";
            DrawText(sp, WIN_WIDTH/2-MeasureText(sp,20)/2, WIN_HEIGHT/2+30, 20, Fade(WHITE, pulse));
            DrawText(tl, WIN_WIDTH/2-MeasureText(tl,14)/2, WIN_HEIGHT/2+60, 14, Fade(COL_GOLD, pulse*0.8f));
        }

        // ── Shop overlay ──────────────────────────────────────────────────
        if (state == SHOP)
            DrawUpgradeShop(player, hoveredUpgrade);

        EndMode2D();
        EndDrawing();
    }

    UnloadSound(sndPlayer);
    UnloadSound(sndAI);
    UnloadSound(sndCollect);
    UnloadSound(sndLevelUp);
    StopMusicStream(music);
    UnloadMusicStream(music);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}