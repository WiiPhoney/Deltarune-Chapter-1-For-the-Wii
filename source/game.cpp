#include "game.h"
#include <stdlib.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <ogc/lwp_watchdog.h>
#include <stdio.h>
#include <asndlib.h>
#include <ogc/audio.h>

extern const u8 room_bg_png[], computer_png[], wagon1_png[];
extern const u8 down1_png[], down2_png[], down3_png[], down4_png[];
extern const u8 left1_png[], left2_png[], left3_png[], left4_png[];
extern const u8 right1_png[], right2_png[], right3_png[], right4_png[];
extern const u8 uo1_png[], uo2_png[], uo3_png[], uo4_png[];
extern const u8 determination_ttf[];
extern const u32 determination_ttf_size;
extern const u8 snd_txttor_raw[];
extern const u32 snd_txttor_raw_size;

static constexpr float ROOM_SCALE = SCALE;

static constexpr float DESK_X = 17.0f;
static constexpr float DESK_Y = 136.0f;
static constexpr float WAGON_X = 256.0f;
static constexpr float WAGON_Y = 147.0f;

static constexpr float BOUND_W = 14.0f * SCALE;      
static constexpr float BOUND_H = 32.0f * SCALE;      
static constexpr float BOUND_XOFF = -7.0f * SCALE;   
static constexpr float BOUND_YOFF = -32.0f * SCALE;  

static inline bool overlaps(const RectF& a, const RectF& b) {
    return (a.x < b.x + b.w &&
            a.x + a.w > b.x &&
            a.y < b.y + b.h &&
            a.y + a.h > b.y);
}



Game::Game() {
    AUDIO_Init(NULL);
    ASND_Init();
    ASND_Pause(0); 
    WPAD_Init();

    WPAD_ControlSpeaker(WPAD_CHAN_0, true);
    
    WPAD_ControlSpeaker(WPAD_CHAN_0, true); 
    
    aSoundPlaying = false;
    wmAudioData = nullptr;
    wmAudioSize = 0;
    wmAudioOffset = 0;

    gameState = STATE_OVERWORLD;

    // Initialize Player 1 (Kris) - Always starts active
    players[0].active = true;
    players[0].x = 300.0f; 
    players[0].y = 350.0f;
    players[0].direction = 0;
    players[0].frame = 0;
    players[0].tick = 0;
    players[0].colorTint = 0xFFFFFFFF; 

    // Initialize Player 2 (Red Kris) - Starts inactive
    players[1].active = false; 
    players[1].x = 380.0f; 
    players[1].y = 350.0f;
    players[1].direction = 0;
    players[1].frame = 0;
    players[1].tick = 0;
    players[1].colorTint = 0xFF8888FF; 
    textCharCount = 0;
    textTick = 0;

    lastFPSTime = gettime();
    frameCount = 0;
    currentFPS = 0;

    loadAssets();

    roomX = (640.0f - (assets.roomTex->w * SCALE)) / 2.0f;
    roomY = (480.0f - (assets.roomTex->h * SCALE)) / 2.0f;
}

Game::~Game() {
    unloadAssets();
}

void Game::playSound(const u8* sound, u32 size) {
    ASND_StopVoice(0);
    ASND_SetVoice(
        0,
        VOICE_STEREO_16BIT,
        44100,
        0,
        (void*)sound,
        size,
        255, 255,
        NULL
    );
}

void Game::playSoundWiimote(const u8* data, u32 size) {
    wmAudioData = data;
    wmAudioSize = size;
    wmAudioOffset = 0;
    aSoundPlaying = true;
}

void Game::updateWiimoteStream() {
    if (!aSoundPlaying || wmAudioData == nullptr) return;

    const u32 chunkSize = 40; 
    if (wmAudioOffset < wmAudioSize) {
        u32 len = std::min(chunkSize, wmAudioSize - wmAudioOffset);
        WPAD_SendStreamData(0, (void*)(wmAudioData + wmAudioOffset), len);
        wmAudioOffset += len;
    } else {
        aSoundPlaying = false;
        wmAudioData = nullptr;
    }
}

void Game::loadAssets() {
    assets.roomTex     = GRRLIB_LoadTexturePNG(room_bg_png);
    assets.computerTex = GRRLIB_LoadTexturePNG(computer_png);
    assets.wagonTex    = GRRLIB_LoadTexturePNG(wagon1_png);

    assets.gameFont = GRRLIB_LoadTTF(determination_ttf, determination_ttf_size);

    if (!assets.gameFont) {
        printf("ERROR: Font failed to load!\n");
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
    
    for (int i = 0; i < 2; i++) {
        u32 type;
        s32 res = WPAD_Probe(i, &type);

        if (res != WPAD_ERR_NONE) {
            players[i].active = (i == 0);
            continue;
        }

        players[i].active = true;

        u32 held = WPAD_ButtonsHeld(i);
        u32 down = WPAD_ButtonsDown(i);
        WPADData *wd = WPAD_Data(i);

        if (!wd) continue;

        bool isHorizontal = true;
        if (abs(wd->accel.y - 128) > abs(wd->accel.x - 128) && wd->accel.y != 0) {
            isHorizontal = false;
        }

        if (!isHorizontal) {
            players[i].input.up    = held & WPAD_BUTTON_RIGHT;
            players[i].input.down  = held & WPAD_BUTTON_LEFT;
            players[i].input.left  = held & WPAD_BUTTON_UP;
            players[i].input.right = held & WPAD_BUTTON_DOWN;
            players[i].input.btnA  = down & WPAD_BUTTON_2;
            players[i].input.btnB  = held & WPAD_BUTTON_1;
        } else {
            players[i].input.up    = held & WPAD_BUTTON_UP;
            players[i].input.down  = held & WPAD_BUTTON_DOWN;
            players[i].input.left  = held & WPAD_BUTTON_LEFT;
            players[i].input.right = held & WPAD_BUTTON_RIGHT;
            players[i].input.btnA  = down & WPAD_BUTTON_A;
            players[i].input.btnB  = held & WPAD_BUTTON_B;
        }
        
    }
}

RectF Game::playerFeetRect(int id) const {
    return {
        players[id].x - (10.0f * SCALE * 0.5f),
        players[id].y - (6.0f * SCALE),
        10.0f * SCALE,
        6.0f * SCALE
    };
}

RectF Game::playerBoundingBox(int id) const {
    return {
        players[id].x + BOUND_XOFF,
        players[id].y + BOUND_YOFF,
        BOUND_W,
        BOUND_H
    };
}

RectF Game::facingRect(int id) const {
    const float reach = 12.0f * SCALE;
    const float thickness = 14.0f * SCALE;
    
    float centerX = players[id].x;
    float centerY = players[id].y - (4.0f * SCALE);

    switch (players[id].direction) {
        case 0: // Down
            return { 
                centerX - (thickness / 2.0f), 
                centerY + (6.0f * SCALE),  
                thickness, 
                reach 
            };
        case 1: // Up
            return { 
                centerX - (thickness / 2.0f), 
                centerY - reach - (20.0f * SCALE), 
                thickness, 
                reach 
            };
        case 2: // Left
            return { 
                centerX - reach - (10.0f * SCALE), 
                centerY - (thickness / 2.0f) - (10.0f * SCALE), 
                reach, 
                thickness 
            };
        case 3: // Right
        default:
            return { 
                centerX + (6.0f * SCALE), 
                centerY - (thickness / 2.0f) - (10.0f * SCALE), 
                reach, 
                thickness 
            };
    }
}

RectF Game::deskRect() const {
    return { roomX + DESK_X * SCALE, roomY + DESK_Y * SCALE, 54.0f * SCALE, 28.0f * SCALE };
}

RectF Game::wagonRect() const {
    return { roomX + WAGON_X * SCALE, roomY + WAGON_Y * SCALE, 40.0f * SCALE, 24.0f * SCALE };
}

std::vector<RectF> Game::buildSolids() const {
    std::vector<RectF> solids;
    solids.reserve(12);

    float leftWall  = roomX + 44.0f * SCALE;
    float rightWall = roomX + 276.0f * SCALE;
    float topWall   = roomY + 115.0f * SCALE;
    float bottomWall = roomY + 225.0f * SCALE;

    float roomRight  = roomX + 640.0f * SCALE;
    float roomBottom = roomY + 480.0f * SCALE;


    solids.push_back({
        roomX,
        roomY,
        leftWall - roomX,
        roomBottom - roomY
    });

    solids.push_back({
        rightWall,
        roomY,
        roomRight - rightWall,
        roomBottom - roomY
    });

    solids.push_back({
        leftWall,
        roomY,
        rightWall - leftWall,
        topWall - roomY
    });

    solids.push_back({
        leftWall,
        bottomWall,
        rightWall - leftWall,
        roomBottom - bottomWall
    });



    solids.push_back({
        roomX + 47.0f * SCALE,
        roomY + 158.0f * SCALE,   
        40.0f * SCALE,
        20.0f * SCALE
    });

    solids.push_back({
        roomX + 231.0f * SCALE,
        roomY + 158.0f * SCALE,   
        40.0f * SCALE,
        20.0f * SCALE
    });

    solids.push_back({
        roomX + 48.0f * SCALE,
        roomY + 118.0f * SCALE,
        40.0f * SCALE,
        22.0f * SCALE
    });

    solids.push_back({
        roomX + 232.0f * SCALE,
        roomY + 118.0f * SCALE,
        40.0f * SCALE,
        22.0f * SCALE
    });

    solids.push_back(deskRect());
    solids.push_back(wagonRect());

    return solids;
}

bool Game::blocked(const RectF& r, const std::vector<RectF>& solids) const {
    for (const auto& s : solids) {
        if (overlaps(r, s)) return true;
    }
    return false;
}

void Game::moveAndCollide(int id, float dx, float dy, const std::vector<RectF>& solids) {
    int steps = std::max(1, (int)std::ceil(std::max(std::fabs(dx), std::fabs(dy))));
    float sx = dx / steps;
    float sy = dy / steps;

    for (int i = 0; i < steps; ++i) {
        players[id].x += sx;
        if (blocked(playerBoundingBox(id), solids)) {
            players[id].x -= sx;
        }
        players[id].y += sy;
        if (blocked(playerBoundingBox(id), solids)) {
            players[id].y -= sy;
        }
    }
}

void Game::updateOverworld() {
    std::vector<RectF> solids = buildSolids();
    for (int i = 0; i < 2; i++) {
        if (!players[i].active) continue;

        float speed = (players[i].input.btnB) ? 4.2f : 2.4f;
        float dx = 0.0f, dy = 0.0f;
        bool moving = false;

        if (players[i].input.left)  { dx -= speed; players[i].direction = 2; moving = true; }
        if (players[i].input.right) { dx += speed; players[i].direction = 3; moving = true; }
        if (players[i].input.up)    { dy -= speed; players[i].direction = 1; moving = true; }
        if (players[i].input.down)  { dy += speed; players[i].direction = 0; moving = true; }

        moveAndCollide(i, dx, dy, solids);

        if (i == 0 && players[i].input.btnA) {
            if (overlaps(facingRect(i), wagonRect())) {
                currentText = "* (An old wagon.)\n* (You feel like you've used it before.)";
                textCharCount = 0;
                textTick = 0;
                gameState = STATE_DIALOGUE;
            }
        }

        if (moving) {
            int animSpeed = (players[i].input.btnB) ? 4 : 7;
            if (++players[i].tick > animSpeed) {
                players[i].frame = (players[i].frame + 1) % 4;
                players[i].tick = 0;
            }
        } else {
            players[i].frame = 0;
            players[i].tick = 0;
        }
    }
}

void Game::render() {
    GRRLIB_FillScreen(0x000000FF); 
    GRRLIB_DrawImg(std::round(roomX), std::round(roomY), assets.roomTex, 0, SCALE, SCALE, 0xFFFFFFFF);

    std::vector<Renderable> scene;
    scene.reserve(10);

    scene.push_back({assets.computerTex, roomX + DESK_X * SCALE, roomY + DESK_Y * SCALE, roomY + DESK_Y * SCALE, false, 0xFFFFFFFF});
    scene.push_back({assets.wagonTex, roomX + WAGON_X * SCALE, roomY + WAGON_Y * SCALE, roomY + WAGON_Y * SCALE, false, 0xFFFFFFFF});

    for (int i = 0; i < 2; i++) {
        if (!players[i].active) continue; 

        GRRLIB_texImg* pTex;
        switch (players[i].direction) {
            case 1: pTex = assets.anim.up[players[i].frame]; break;
            case 2: pTex = assets.anim.left[players[i].frame]; break;
            case 3: pTex = assets.anim.right[players[i].frame]; break;
            default: pTex = assets.anim.down[players[i].frame]; break;
        }

        scene.push_back({pTex, players[i].x, players[i].y, players[i].y, true, players[i].colorTint});
    }

    std::sort(scene.begin(), scene.end(), [](const Renderable& a, const Renderable& b) {
        return a.sortY < b.sortY;
    });

    for (auto& obj : scene) {
        if (!obj.tex) continue; 
        float drawX, drawY;
        if (obj.isPlayer) {
            drawX = std::round(obj.x - (obj.tex->w * SCALE / 2.0f));
            drawY = std::round(obj.y - (obj.tex->h * SCALE));
        } else {
            drawX = std::round(obj.x);
            drawY = std::round(obj.y);
        }
        GRRLIB_DrawImg(drawX, drawY, obj.tex, 0, SCALE, SCALE, obj.color);
    }

    if (gameState == STATE_DIALOGUE) {
        drawDialogueBox(currentText);
    }

void Game::drawDialogueBox(std::string text) {
    GRRLIB_Rectangle(50, 320, 540, 120, 0x000000FF, 1);
    GRRLIB_Rectangle(55, 325, 530, 110, 0xFFFFFFFF, 0);

    textTick++;
    if (textTick >= 2) {
        if (textCharCount < (int)text.length()) {
            textCharCount++;
            
            playSoundWiimote(snd_txttor_raw, snd_txttor_raw_size);
        }
        textTick = 0;
    }

    std::string visibleText = text.substr(0, textCharCount);

    if (assets.gameFont) {
        std::string line1, line2;
        size_t newlinePos = visibleText.find('\n');
        
        if (newlinePos != std::string::npos) {
            line1 = visibleText.substr(0, newlinePos);
            if (newlinePos + 1 < visibleText.length()) {
                line2 = visibleText.substr(newlinePos + 1);
            }
            
            GRRLIB_PrintfTTF(75, 345, assets.gameFont, line1.c_str(), 20, 0x000000FF);
            GRRLIB_PrintfTTF(75, 375, assets.gameFont, line2.c_str(), 20, 0x000000FF);
        } else {
            GRRLIB_PrintfTTF(75, 345, assets.gameFont, visibleText.c_str(), 20, 0x000000FF);
        }
    } else {
        GRRLIB_Printf(75, 345, NULL, 1.0f, 0x000000FF, "%s", visibleText.c_str());
    }
}

void Game::update() {
    readInput();

    updateWiimoteStream();

    if (gameState == STATE_OVERWORLD) {
        updateOverworld();
    }
    else if (gameState == STATE_DIALOGUE) {

        if (players[0].input.btnA) {

            if (textCharCount < (int)currentText.length()) {
                textCharCount = (int)currentText.length();
                textTick = 0;
            }
            else {
                gameState = STATE_OVERWORLD;
                textCharCount = 0;
                textTick = 0;

                aSoundPlaying = false;
                wmAudioData = nullptr;
            }
        }
    }

    updateFPS();
}

bool Game::checkCollision(float px, float py, float pw, float ph,
                          float ox, float oy, float ow, float oh) {
    return (px < ox+ow && px+pw > ox && py < oy+oh && py+ph > oy);
}

void Game::updateFPS() {
    frameCount++;
    u32 now = gettime();
    u32 elapsed = ticks_to_millisecs(now - lastFPSTime);
    if (elapsed >= 1000) {
        currentFPS = frameCount;
        frameCount = 0;
        lastFPSTime = now;
    }
}

void Game::unloadAssets() {
    GRRLIB_FreeTexture(assets.roomTex);
    GRRLIB_FreeTexture(assets.computerTex);
    GRRLIB_FreeTTF(assets.gameFont);
    GRRLIB_FreeTexture(assets.wagonTex);
    for (int i = 0; i < 4; i++) {
        GRRLIB_FreeTexture(assets.anim.down[i]);
        GRRLIB_FreeTexture(assets.anim.up[i]);
        GRRLIB_FreeTexture(assets.anim.left[i]);
        GRRLIB_FreeTexture(assets.anim.right[i]);
    }
}
