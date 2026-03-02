#include "HUD.h"
#include "Constants.h"
#include "Upgrades.h"
#include <algorithm>
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
// Low-level helpers
// ─────────────────────────────────────────────────────────────────────────────
void DrawOutlinedRect(Rectangle r, float roundness, Color fill, Color outline, float thick) {
    Rectangle o = { r.x-thick, r.y-thick, r.width+thick*2, r.height+thick*2 };
    DrawRectangleRounded(o, roundness, 8, outline);
    DrawRectangleRounded(r, roundness, 8, fill);
}

void DrawOutlinedCircle(Vector2 pos, float r, Color fill, Color outline, float thick) {
    DrawCircleV(pos, r+thick, outline);
    DrawCircleV(pos, r, fill);
}

void DrawOutlinedText(const char* text, int x, int y, int fs, Color col, int ow) {
    for (int ox=-ow; ox<=ow; ox++) for (int oy=-ow; oy<=ow; oy++) {
        if (ox==0 && oy==0) continue;
        DrawText(text, x+ox, y+oy, fs, COL_OUTLINE);
    }
    DrawText(text, x, y, fs, col);
}

// ─────────────────────────────────────────────────────────────────────────────
// Stat panel
// ─────────────────────────────────────────────────────────────────────────────
void DrawStatPanel(int x, int y, const char* label, const char* value,
                   Color accent, bool highlight) {
    int pw=162, ph=54;
    DrawOutlinedRect({(float)x,(float)y,(float)pw,(float)ph}, 0.35f,
                     {20,22,45,220}, COL_OUTLINE, 2.5f);
    DrawRectangleRounded({(float)x+2,(float)y,(float)(pw-4),4.0f}, 0.5f, 4, accent);
    if (highlight)
        DrawRectangleRounded({(float)(x-1),(float)(y-1),(float)(pw+2),(float)(ph+2)},
                             0.35f, 8, Fade(accent, 0.15f));
    DrawOutlinedText(label, x+10, y+8,  11, Fade(accent, 0.9f), 1);
    DrawOutlinedText(value, x+10, y+24, 20, WHITE, 2);
}

// ─────────────────────────────────────────────────────────────────────────────
// Difficulty bar
// ─────────────────────────────────────────────────────────────────────────────
void DrawDifficultyBar(float factor) {
    float t  = std::max(0.0f, std::min(1.0f,
                  (factor-AI_SPEED_MIN)/(AI_SPEED_MAX-AI_SPEED_MIN)));
    int bw=130, bh=10, bx=WIN_WIDTH/2-65, by=80;
    DrawOutlinedRect({(float)bx,(float)by,(float)bw,(float)bh},
                     0.5f, {30,32,58,220}, COL_OUTLINE, 2.0f);
    if (bw*t >= 1.0f) {
        Color fill = (t < 0.5f)
            ? Color{(unsigned char)(80+(int)(175*t*2)), 220, 80, 255}
            : Color{255, (unsigned char)(220-(int)(220*(t-0.5f)*2)), 80, 255};
        DrawRectangleRounded({(float)bx,(float)by,bw*t,(float)bh}, 0.5f, 4, fill);
    }
    int lw = MeasureText("AI DIFFICULTY", 10);
    DrawText("AI DIFFICULTY", WIN_WIDTH/2-lw/2, by-14, 10, Fade(WHITE, 0.45f));
}

// ─────────────────────────────────────────────────────────────────────────────
// XP bar  (sits just above the bottom HUD panels)
// ─────────────────────────────────────────────────────────────────────────────
void DrawXPBar(const Player& player) {
    int bw = WIN_WIDTH - 28, bh = 8;
    int bx = 14, by = WIN_HEIGHT - 84;

    // Background track
    DrawOutlinedRect({(float)bx,(float)by,(float)bw,(float)bh},
                     0.5f, {25,28,55,220}, COL_OUTLINE, 1.5f);

    // Fill
    float pct  = (player.xpToNext > 0.0f) ? (player.xp / player.xpToNext) : 0.0f;
    pct = std::max(0.0f, std::min(pct, 1.0f));
    if (bw * pct >= 1.0f)
        DrawRectangleRounded({(float)bx,(float)by, bw*pct,(float)bh},
                             0.5f, 4, COL_XP);

    // Label  e.g.  "LVL 3  |  45 / 210 XP"
    const char* lbl = TextFormat("LVL %d   %d / %d XP",
                                  player.level,
                                  (int)player.xp,
                                  (int)player.xpToNext);
    int lw = MeasureText(lbl, 11);
    DrawText(lbl, WIN_WIDTH/2 - lw/2, by - 14, 11, Fade(COL_XP, 0.85f));
}

// ─────────────────────────────────────────────────────────────────────────────
// Shield pips
// ─────────────────────────────────────────────────────────────────────────────
void DrawShieldPips(int shieldHP, int upgShieldLevel, float playerMoney, int state) {
    // state == 1 means WAITING (show cost hint)
    for (int s = 0; s < shieldHP; s++)
        DrawOutlinedCircle(
            { (float)(WIN_WIDTH/2 - (shieldHP-1)*13 + s*26), (float)(WIN_HEIGHT-18) },
            7.0f, COL_GOLD, COL_OUTLINE, 2.0f);

    if (upgShieldLevel > 0 && state == 0 /*WAITING*/) {
        int cost = (int)(SHIELD_USE_COST * upgShieldLevel);
        const char* sl = TextFormat("shields: %d c", cost);
        DrawText(sl, WIN_WIDTH/2 - MeasureText(sl,11)/2, WIN_HEIGHT-32, 11,
                 playerMoney >= cost ? Fade(COL_GOLD, 0.7f) : Fade(COL_RED, 0.8f));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Upgrade shop
// ─────────────────────────────────────────────────────────────────────────────
void DrawUpgradeShop(const Player& player, int& hovered) {
    DrawRectangle(0, 0, WIN_WIDTH, WIN_HEIGHT, {0,0,0,175});
    int pw=500, ph=520, px=WIN_WIDTH/2-250, py=WIN_HEIGHT/2-260;
    DrawOutlinedRect({(float)px,(float)py,(float)pw,(float)ph},
                     0.12f, {15,18,38,248}, COL_GOLD, 3.5f);

    int tw = MeasureText("UPGRADE SHOP", 28);
    DrawOutlinedText("UPGRADE SHOP", WIN_WIDTH/2-tw/2, py+14, 28, COL_GOLD, 2);

    const char* coinStr = TextFormat("Available: %.0f coins", player.money);
    int cw = MeasureText(coinStr, 15);
    DrawText(coinStr, WIN_WIDTH/2-cw/2, py+52, 15, COL_GREEN);

    // Section header for upgrades
    DrawText("UPGRADES", px+20, py+80, 14, Fade(COL_GOLD, 0.8f));

    hovered = -1;
    int mx = GetMouseX(), my = GetMouseY();

    // Draw upgrades
    for (int i = 0; i < UPG_COUNT; i++) {
        int bx=px+20, by2=py+100+i*58, bw2=pw-40, bh2=50;
        bool maxed  = (player.upgLevels[i] >= UPGRADES[i].maxLevel);
        float cost  = maxed ? 0 : UpgradeCost((UpgradeID)i, player.upgLevels[i]);
        bool canBuy = (!maxed && player.money >= cost);
        bool over   = (mx>=bx && mx<=bx+bw2 && my>=by2 && my<=by2+bh2);
        if (over) hovered = i;

        Color bg   = (over && canBuy) ? Color{30,40,80,255} : Color{20,24,50,240};
        Color bord = maxed ? COL_GREEN : (canBuy ? UPGRADES[i].accent : Color{60,60,80,255});
        DrawOutlinedRect({(float)bx,(float)by2,(float)bw2,(float)bh2},
                         0.25f, bg, bord, 2.5f);
        DrawOutlinedText(UPGRADES[i].name, bx+10, by2+7,  16, maxed ? COL_GREEN : WHITE, 1);
        DrawText(UPGRADES[i].desc,         bx+10, by2+28, 11, Fade(WHITE, 0.55f));

        // Level pips
        for (int l = 0; l < UPGRADES[i].maxLevel; l++) {
            Color dc = (l < player.upgLevels[i]) ? UPGRADES[i].accent : Color{40,40,70,255};
            DrawOutlinedCircle({(float)(bx+bw2-18-l*16),(float)(by2+14)},
                               5.0f, dc, COL_OUTLINE, 1.5f);
        }

        if (maxed)
            DrawText("MAXED", bx+bw2-58, by2+31, 13, COL_GREEN);
        else
            DrawText(TextFormat("%.0f c", cost), bx+bw2-60, by2+31, 13,
                     canBuy ? COL_GOLD : COL_RED);
    }

    // Section header for abilities
    DrawText("ABILITIES", px+20, py+276, 14, Fade({ 100, 200, 255, 255 }, 0.8f));

    // Draw abilities
    for (int i = 0; i < ABL_COUNT; i++) {
        int bx=px+20, by2=py+296+i*58, bw2=pw-40, bh2=50;
        bool owned  = player.ownedAbilities[i];
        float cost  = ABILITIES[i].cost;
        bool canBuy = (!owned && player.money >= cost);
        bool over   = (mx>=bx && mx<=bx+bw2 && my>=by2 && my<=by2+bh2);
        if (over) hovered = UPG_COUNT + i;

        Color bg   = (over && canBuy) ? Color{30,40,80,255} : Color{20,24,50,240};
        Color bord = owned ? COL_GREEN : (canBuy ? ABILITIES[i].accent : Color{60,60,80,255});
        DrawOutlinedRect({(float)bx,(float)by2,(float)bw2,(float)bh2},
                         0.25f, bg, bord, 2.5f);
        DrawOutlinedText(ABILITIES[i].name, bx+10, by2+7,  16, owned ? COL_GREEN : WHITE, 1);
        DrawText(ABILITIES[i].desc,         bx+10, by2+28, 11, Fade(WHITE, 0.55f));

        if (owned)
            DrawText("OWNED", bx+bw2-58, by2+31, 13, COL_GREEN);
        else
            DrawText(TextFormat("%.0f c", cost), bx+bw2-60, by2+31, 13,
                     canBuy ? COL_GOLD : COL_RED);
    }

    int fw = MeasureText("[ Click to buy ]   [ TAB to close ]", 12);
    DrawText("[ Click to buy ]   [ TAB to close ]",
             WIN_WIDTH/2-fw/2, py+ph-22, 12, Fade(WHITE, 0.4f));
}