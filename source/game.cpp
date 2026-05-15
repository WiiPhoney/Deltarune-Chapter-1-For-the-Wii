#include "game.h"
#include <stdlib.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <ogc/lwp_watchdog.h>
#include <stdio.h>
#include <string.h>

#define DEBUG_COLLISION

extern const u8 room_bg_png[], computer_png[], wagon1_png[];
extern const u8 down1_png[], down2_png[], down3_png[], down4_png[];
extern const u8 left1_png[], left2_png[], left3_png[], left4_png[];
extern const u8 right1_png[], right2_png[], right3_png[], right4_png[];
extern const u8 uo1_png[], uo2_png[], uo3_png[], uo4_png[];
extern const u8 determination_ttf[];
extern const u32 determination_ttf_size;
extern const u8 snd_txttor_raw[];
extern const u32 snd_txttor_raw_size;

static const float ROOM_SCALE = SCALE;

static const float DESK_X = 17.0f;
static const float DESK_Y = 136.0f;

static const float WAGON_X = 256.0f;
static const float WAGON_Y = 147.0f;

static const float BOUND_W = 12.0f * SCALE;
static const float BOUND_H = 12.0f * SCALE;

static const float BOUND_XOFF = -(BOUND_W / 2.0f);
static const float BOUND_YOFF = -BOUND_H;

static inline bool overlaps(const RectF& a, const RectF& b) {
    return (
        a.x < b.x + b.w &&
        a.x + a.w > b.x &&
        a.y < b.y + b.h &&
        a.y + a.h > b.y
    );
}

Game::Game() {
    memset(&assets, 0, sizeof(assets));
    memset(players, 0, sizeof(players));

    aSoundPlaying = false;
    wmAudioData = NULL;
    wmAudioSize = 0;
    wmAudioOffset = 0;

    gameState = STATE_OVERWORLD;

    textCharCount = 0;
    textTick = 0;

    roomX = 160.0f;
    roomY = 60.0f;

    frameCount = 0;
    currentFPS = 0;
    lastFPSTime = 0;

    // PLAYER 1
    players[0].active = true;
    players[0].x = 160.0f;
    players[0].y = 210.0f;
    players[0].direction = 0;
    players[0].frame = 0;
    players[0].tick = 0;
    players[0].colorTint = 0xFFFFFFFF;

    // PLAYER 2
    players[1].active = false;
    players[1].x = 200.0f;
    players[1].y = 210.0f;
    players[1].direction = 0;
    players[1].frame = 0;
    players[1].tick = 0;
    players[1].colorTint = 0xFF8888FF;

    lastFPSTime = gettime();

    loadAssets();

    if (assets.roomTex != NULL) {
        roomX = (640.0f - (assets.roomTex->w * SCALE)) / 2.0f;
        roomY = (480.0f - (assets.roomTex->h * SCALE)) / 2.0f;
    }
}

Game::~Game() {
    unloadAssets();
}

void Game::playSound(const u8* sound, u32 size) {
    return;
}

void Game::playSoundWiimote(const u8* data, u32 size) {
    if (!data || size == 0) return;

    wmAudioData = data;
    wmAudioSize = size;
    wmAudioOffset = 0;
    aSoundPlaying = true;
}

void Game::updateWiimoteStream() {
    if (!aSoundPlaying || wmAudioData == NULL) return;

    const u32 chunkSize = 40;

    if (wmAudioOffset < wmAudioSize) {
        u32 len = chunkSize;

        if (wmAudioOffset + chunkSize > wmAudioSize) {
            len = wmAudioSize - wmAudioOffset;
        }

        WPAD_SendStreamData(
            0,
            (void*)(wmAudioData + wmAudioOffset),
            len
        );

        wmAudioOffset += len;
    } else {
        aSoundPlaying = false;
        wmAudioData = NULL;
    }
}

void Game::loadAssets() {
    assets.roomTex = GRRLIB_LoadTexturePNG(room_bg_png);
    assets.computerTex = GRRLIB_LoadTexturePNG(computer_png);
    assets.wagonTex = GRRLIB_LoadTexturePNG(wagon1_png);

    assets.gameFont = GRRLIB_LoadTTF(
        determination_ttf,
        determination_ttf_size
    );

    for (int i = 0; i < 4; i++) {
        assets.anim.down[i] = NULL;
        assets.anim.up[i] = NULL;
        assets.anim.left[i] = NULL;
        assets.anim.right[i] = NULL;
    }

    assets.anim.down[0] = GRRLIB_LoadTexturePNG(down1_png);
    assets.anim.down[1] = GRRLIB_LoadTexturePNG(down2_png);
    assets.anim.down[2] = GRRLIB_LoadTexturePNG(down3_png);
    assets.anim.down[3] = GRRLIB_LoadTexturePNG(down4_png);

    assets.anim.up[0] = GRRLIB_LoadTexturePNG(uo1_png);
    assets.anim.up[1] = GRRLIB_LoadTexturePNG(uo2_png);
    assets.anim.up[2] = GRRLIB_LoadTexturePNG(uo3_png);
    assets.anim.up[3] = GRRLIB_LoadTexturePNG(uo4_png);

    assets.anim.left[0] = GRRLIB_LoadTexturePNG(left1_png);
    assets.anim.left[1] = GRRLIB_LoadTexturePNG(left2_png);
    assets.anim.left[2] = GRRLIB_LoadTexturePNG(left3_png);
    assets.anim.left[3] = GRRLIB_LoadTexturePNG(left4_png);

    assets.anim.right[0] = GRRLIB_LoadTexturePNG(right1_png);
    assets.anim.right[1] = GRRLIB_LoadTexturePNG(right2_png);
    assets.anim.right[2] = GRRLIB_LoadTexturePNG(right3_png);
    assets.anim.right[3] = GRRLIB_LoadTexturePNG(right4_png);
}

void Game::readInput() {
    WPAD_ScanPads();

    for (int i = 0; i < 2; i++) {
        players[i].input.up = false;
        players[i].input.down = false;
        players[i].input.left = false;
        players[i].input.right = false;
        players[i].input.btnA = false;
        players[i].input.btnB = false;

        u32 type;

        if (WPAD_Probe(i, &type) != WPAD_ERR_NONE) {
            players[i].active = (i == 0);
            continue;
        }

        players[i].active = true;

        u32 held = WPAD_ButtonsHeld(i);
        u32 down = WPAD_ButtonsDown(i);

        players[i].input.up = held & WPAD_BUTTON_UP;
        players[i].input.down = held & WPAD_BUTTON_DOWN;
        players[i].input.left = held & WPAD_BUTTON_LEFT;
        players[i].input.right = held & WPAD_BUTTON_RIGHT;
        players[i].input.btnA = down & WPAD_BUTTON_A;
        players[i].input.btnB = held & WPAD_BUTTON_B;
    }
}

RectF Game::playerBoundingBox(int id) const {
    return RectF{
        players[id].x - (BOUND_W * 0.5f),
        players[id].y - BOUND_H,
        BOUND_W,
        BOUND_H
    };
}

RectF Game::facingRect(int id) const {
    const float reach = 16.0f * SCALE;
    const float size = 14.0f * SCALE;

    float px = players[id].x;
    float py = players[id].y - (BOUND_H * 0.5f);

    switch (players[id].direction) {
        case 0:
            return RectF{
                px - size * 0.5f,
                py + 2.0f * SCALE,
                size,
                reach
            };

        case 1:
            return RectF{
                px - size * 0.5f,
                py - reach,
                size,
                reach
            };

        case 2:
            return RectF{
                px - reach,
                py - size * 0.5f,
                reach,
                size
            };

        default:
            return RectF{
                px,
                py - size * 0.5f,
                reach,
                size
            };
    }
}

RectF Game::deskRect() const {
    return RectF{
        roomX + (DESK_X + 6.0f) * SCALE,
        roomY + (DESK_Y + 12.0f) * SCALE,
        42.0f * SCALE,
        12.0f * SCALE
    };
}

RectF Game::wagonRect() const {
    return RectF{
        roomX + (WAGON_X + 4.0f) * SCALE,
        roomY + (WAGON_Y + 10.0f) * SCALE,
        30.0f * SCALE,
        10.0f * SCALE
    };
}

std::vector<RectF> Game::buildSolids() const {
    std::vector<RectF> solids;

    float lWall = roomX + 44.0f * SCALE;
    float rWall = roomX + 276.0f * SCALE;

    float tWall = roomY + 115.0f * SCALE;
    float bWall = roomY + 225.0f * SCALE;

    float doorLeft = roomX + 145.0f * SCALE;
    float doorRight = roomX + 175.0f * SCALE;

    solids.push_back(RectF{
        roomX,
        roomY,
        lWall - roomX,
        480.0f
    });

    solids.push_back(RectF{
        rWall,
        roomY,
        640.0f - rWall,
        480.0f
    });

    solids.push_back(RectF{
        lWall,
        roomY,
        rWall - lWall,
        tWall - roomY
    });

    solids.push_back(RectF{
        lWall,
        bWall,
        doorLeft - lWall,
        480.0f - bWall
    });

    solids.push_back(RectF{
        doorRight,
        bWall,
        rWall - doorRight,
        480.0f - bWall
    });

    solids.push_back(deskRect());
    solids.push_back(wagonRect());

    return solids;
}

bool Game::blocked(
    const RectF& r,
    const std::vector<RectF>& solids
) const {
    for (size_t i = 0; i < solids.size(); i++) {
        if (overlaps(r, solids[i])) {
            return true;
        }
    }

    return false;
}

void Game::moveAndCollide(
    int id,
    float dx,
    float dy,
    const std::vector<RectF>& solids
) {
    players[id].x += dx;

    if (blocked(playerBoundingBox(id), solids)) {
        players[id].x -= dx;
    }

    players[id].y += dy;

    if (blocked(playerBoundingBox(id), solids)) {
        players[id].y -= dy;
    }
}

void Game::updateOverworld() {
    std::vector<RectF> solids = buildSolids();

    for (int i = 0; i < 2; i++) {
        if (!players[i].active) continue;

        float speed = players[i].input.btnB ? 4.2f : 2.4f;

        float dx = 0.0f;
        float dy = 0.0f;

        bool moving = false;

        if (players[i].input.left) {
            dx -= speed;
            players[i].direction = 2;
            moving = true;
        }

        if (players[i].input.right) {
            dx += speed;
            players[i].direction = 3;
            moving = true;
        }

        if (players[i].input.up) {
            dy -= speed;
            players[i].direction = 1;
            moving = true;
        }

        if (players[i].input.down) {
            dy += speed;
            players[i].direction = 0;
            moving = true;
        }

        moveAndCollide(i, dx, dy, solids);

        if (
            i == 0 &&
            players[i].input.btnA &&
            overlaps(facingRect(i), wagonRect())
        ) {
            currentText =
                "* (An old wagon.)\n"
                "* (You feel like you've used it before.)";

            textCharCount = 0;
            textTick = 0;

            gameState = STATE_DIALOGUE;
        }

        if (moving) {
            int tickThreshold = players[i].input.btnB ? 4 : 7;

            players[i].tick++;

            if (players[i].tick > tickThreshold) {
                players[i].frame =
                    (players[i].frame + 1) % 4;

                players[i].tick = 0;
            }
        } else {
            players[i].frame = 0;
        }
    }
}