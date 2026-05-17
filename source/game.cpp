#include "game.h"
#include <stdlib.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <ogc/lwp_watchdog.h>
#include <stdio.h>
#include <asndlib.h>
#include <ogc/audio.h>
#include <ogc/pad.h>
#include <ogc/si.h>
#include <ogc/system.h>


extern const u8 room_bg_png[], computer_png[], wagon1_png[];
extern const u8 room_bg2_png[];
extern const u8 down1_png[], down2_png[], down3_png[], down4_png[];
extern const u8 left1_png[], left2_png[], left3_png[], left4_png[];
extern const u8 right1_png[], right2_png[], right3_png[], right4_png[];
extern const u8 uo1_png[], uo2_png[], uo3_png[], uo4_png[];
extern const u8 determination_ttf[];
extern const u32 determination_ttf_size;
extern const u8 snd_txttor_raw[];
extern const u32 snd_txttor_raw_size;

// Updated collision constants based on GameMaker Room3 data
static constexpr float DESK_X = 42.0f;      // Computer position
static constexpr float DESK_Y = 144.0f;
static constexpr float WAGON_X = 242.0f;
static constexpr float WAGON_Y = 164.0f;
static constexpr float HITBOX_W = 8.0f; 
static constexpr float HITBOX_H = 6.0f;

static inline bool overlaps(const RectF& a, const RectF& b) {
    return (a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y);
}

Game::Game() {
    ASND_Pause(0); 
    WPAD_ControlSpeaker(WPAD_CHAN_0, false);
    PAD_Init(); 
    aSoundPlaying = false;
    wmAudioData = nullptr;
    wmAudioSize = 0;
    wmAudioOffset = 0;
    gameState = STATE_OVERWORLD;
    currentRoom = ROOM_1;
    warpCooldown = 60;
    currentRoomScale = 2.0f; 
    camX = 0;
    camY = 0;
    loadAssets();
    roomX = (640.0f - (assets.roomTex->w * currentRoomScale)) / 2.0f;
    roomY = (480.0f - (assets.roomTex->h * currentRoomScale)) / 2.0f;
    players[0].active = true;
    players[0].x = roomX + (160.0f * currentRoomScale); 
    players[0].y = roomY + (160.0f * currentRoomScale);
    players[0].direction = 0;
    players[0].frame = 0;
    players[0].tick = 0;
    players[0].colorTint = 0xFFFFFFFF; 
    players[0].input.left = players[0].input.right = players[0].input.up = players[0].input.down = players[0].input.btnA = players[0].input.btnB = false;
    players[1].active = false; 
    players[1].x = 380.0f; 
    players[1].y = 240.0f;
    players[1].direction = 0;
    players[1].frame = 0;
    players[1].tick = 0;
    players[1].colorTint = 0xFF8888FF; 
    players[1].input.left = players[1].input.right = players[1].input.up = players[1].input.down = players[1].input.btnA = players[1].input.btnB = false;
    textCharCount = 0;
    textTick = 0;
    lastFPSTime = gettime();
    frameCount = 0;
    currentFPS = 0;
}

Game::~Game() {
    unloadAssets();
}

void Game::playSound(const u8* sound, u32 size) {
    if (sound == nullptr) return;
    ASND_StopVoice(0);
    ASND_SetVoice(0, VOICE_STEREO_16BIT, 44100, 0, (void*)sound, size, 255, 255, NULL);
}

void Game::playSoundWiimote(const u8* data, u32 size) {
    wmAudioData = data;
    wmAudioSize = size;
    wmAudioOffset = 0;
    aSoundPlaying = false;
}

void Game::updateWiimoteStream() {
    if (!aSoundPlaying || wmAudioData == nullptr) return;
    const u32 chunkSize = 40; 
    if (wmAudioOffset < wmAudioSize) {
        u32 len = std::min(chunkSize, wmAudioSize - wmAudioOffset);
        wmAudioOffset += len;
    } else {
        aSoundPlaying = false;
        wmAudioData = nullptr;
    }
}

void Game::loadAssets() {
    assets.roomTex = (currentRoom == ROOM_1) ? GRRLIB_LoadTexturePNG(room_bg_png) : GRRLIB_LoadTexturePNG(room_bg2_png);
    if (assets.roomTex && assets.roomTex->w > 400) {
        currentRoomScale = 1.0f;
    } else {
        currentRoomScale = 2.0f;
    }
    assets.computerTex = GRRLIB_LoadTexturePNG(computer_png);
    assets.wagonTex = GRRLIB_LoadTexturePNG(wagon1_png);
    assets.gameFont = GRRLIB_LoadTTF(determination_ttf, determination_ttf_size);
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
    PAD_ScanPads();
    WPAD_ScanPads();
    
    for (int i = 0; i < 2; i++) {
        players[i].input.up    = false;
        players[i].input.down  = false;
        players[i].input.left  = false;
        players[i].input.right = false;
        players[i].input.btnA  = false;
        players[i].input.btnB  = false;

        // 1. GAMECUBE HARDWARE POLLED MODES
        u16 gcButtons = PAD_ButtonsHeld(i);
        u16 gcDown = PAD_ButtonsDown(i);
        s8 stickX = PAD_StickX(i);
        s8 stickY = PAD_StickY(i);

        if (gcButtons != 0 || gcDown != 0 || stickX != 0 || stickY != 0) {
            players[i].active = true;
            
            const s8 STICK_THRESHOLD = 30;
            bool isStandardGamepad = (abs(stickX) > STICK_THRESHOLD || abs(stickY) > STICK_THRESHOLD || (stickX != 0 || stickY != 0));

            if (isStandardGamepad) {
                // MODE A: STANDARD GAMECUBE CONTROLLER
                bool usingAnalog = (abs(stickX) > STICK_THRESHOLD || abs(stickY) > STICK_THRESHOLD);
                
                if (usingAnalog) {
                    players[i].input.left  = (stickX < -STICK_THRESHOLD);
                    players[i].input.right = (stickX > STICK_THRESHOLD);
                    players[i].input.up    = (stickY > STICK_THRESHOLD);
                    players[i].input.down  = (stickY < -STICK_THRESHOLD);
                } else {
                    players[i].input.up    = (gcButtons & PAD_BUTTON_UP);
                    players[i].input.down  = (gcButtons & PAD_BUTTON_DOWN);
                    players[i].input.left  = (gcButtons & PAD_BUTTON_LEFT);
                    players[i].input.right = (gcButtons & PAD_BUTTON_RIGHT);
                }
                
                players[i].input.btnA  = gcDown & PAD_BUTTON_A;
                players[i].input.btnB  = gcButtons & PAD_BUTTON_B;
            } 
            else {
                // MODE B: DANCE MAT (DDR PAD) DETECTED
                bool padUp    = (gcButtons & PAD_BUTTON_UP) != 0;
                bool padDown  = (gcButtons & PAD_BUTTON_DOWN) != 0;
                bool padLeft  = (gcButtons & PAD_BUTTON_LEFT) != 0;
                bool padRight = (gcButtons & PAD_BUTTON_RIGHT) != 0;

                if (padLeft && padRight) {
                    padLeft = false;
                    padRight = false;
                }
                if (padUp && padDown) {
                    padUp = false;
                    padDown = false;
                }

                players[i].input.up    = padUp;
                players[i].input.down  = padDown;
                players[i].input.left  = padLeft;
                players[i].input.right = padRight;
                
                players[i].input.btnA  = gcDown & PAD_BUTTON_A;
                players[i].input.btnB  = gcButtons & PAD_BUTTON_B;
            }
            
            continue;
        }

        // 2. WII REMOTE & EXTENSIONS MODE
        u32 wpad_type; 
        s32 res = WPAD_Probe(i, &wpad_type);
        if (res != WPAD_ERR_NONE) {
            players[i].active = (i == 0); 
            continue;
        }
        
        players[i].active = true;
        u32 held = WPAD_ButtonsHeld(i);
        u32 down = WPAD_ButtonsDown(i);
        WPADData *wd = WPAD_Data(i); 
        if (!wd) continue;
        
        if (wpad_type == WPAD_EXP_CLASSIC) {
            bool classicUp    = (held & WPAD_CLASSIC_BUTTON_UP) != 0;
            bool classicDown  = (held & WPAD_CLASSIC_BUTTON_DOWN) != 0;
            bool classicLeft  = (held & WPAD_CLASSIC_BUTTON_LEFT) != 0;
            bool classicRight = (held & WPAD_CLASSIC_BUTTON_RIGHT) != 0;

            s16 cStickX = wd->exp.classic.ljs.pos.x;
            s16 cStickY = wd->exp.classic.ljs.pos.y;
            
            const s16 CLASSIC_STICK_THRESHOLD = 15; 
            bool usingAnalog = (abs(cStickX) > CLASSIC_STICK_THRESHOLD || abs(cStickY) > CLASSIC_STICK_THRESHOLD);

            if (usingAnalog) {
                players[i].input.left  = (cStickX < -CLASSIC_STICK_THRESHOLD);
                players[i].input.right = (cStickX > CLASSIC_STICK_THRESHOLD);
                players[i].input.up    = (cStickY > CLASSIC_STICK_THRESHOLD);
                players[i].input.down  = (cStickY < -CLASSIC_STICK_THRESHOLD);
            } else {
                players[i].input.up    = classicUp;
                players[i].input.down  = classicDown;
                players[i].input.left  = classicLeft;
                players[i].input.right = classicRight;
            }
            
            players[i].input.btnA  = (down & WPAD_CLASSIC_BUTTON_A);
            players[i].input.btnB  = (held & WPAD_CLASSIC_BUTTON_B);
        } 
        else {
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
}
RectF Game::playerFeetRect(int id) const {
    return playerBoundingBox(id);
}

RectF Game::playerBoundingBox(int id) const {
    float s = currentRoomScale;
    return { players[id].x - (HITBOX_W * s / 2.0f), players[id].y - (HITBOX_H * s), HITBOX_W * s, HITBOX_H * s };
}

RectF Game::facingRect(int id) const {
    const float reach = 10.0f * currentRoomScale;
    const float thickness = 10.0f * currentRoomScale;
    float centerX = players[id].x;
    float centerY = players[id].y - (4.0f * currentRoomScale);
    switch (players[id].direction) {
        case 0: return { centerX - (thickness / 2.0f), centerY + (4.0f * currentRoomScale), thickness, reach };
        case 1: return { centerX - (thickness / 2.0f), centerY - reach - (18.0f * currentRoomScale), thickness, reach };
        case 2: return { centerX - reach - (8.0f * currentRoomScale), centerY - (thickness / 2.0f) - (8.0f * currentRoomScale), reach, thickness };
        default: return { centerX + (4.0f * currentRoomScale), centerY - (thickness / 2.0f) - (8.0f * currentRoomScale), reach, thickness };
    }
}

RectF Game::deskRect() const { 
    return { roomX + DESK_X * currentRoomScale, roomY + DESK_Y * currentRoomScale, 20.0f * currentRoomScale, 16.0f * currentRoomScale }; 
}

RectF Game::wagonRect() const { 
    return { roomX + WAGON_X * currentRoomScale, roomY + WAGON_Y * currentRoomScale, 40.0f * currentRoomScale, 18.0f * currentRoomScale }; 
}

RectF Game::warpRect() const {
    return { roomX + (144.0f * currentRoomScale), roomY + (211.0f * currentRoomScale), 21.0f * currentRoomScale, 12.0f * currentRoomScale };
}

std::vector<RectF> Game::buildSolids() const {
    std::vector<RectF> solids;
    const float s = currentRoomScale;
    auto rectF = [&](float x, float y, float w, float h) -> RectF { return { roomX + (x * s), roomY + (y * s), w * s, h * s }; };
    
    if (currentRoom == ROOM_1) {
        // Top wall: x:32, y:64, scaleX:12.5, scaleY:0.7692308 (base size 32x32)
        solids.push_back(rectF(32, 64, 400, 25));  // 12.5 * 32 = 400 width
        
        // Left wall: x:21, y:78, scaleX:1.0, scaleY:4.6538463
        solids.push_back(rectF(21, 78, 32, 149));  // 4.6538463 * 32 ≈ 149 height
        
        // Right wall: x:277, y:81, scaleX:1.0, scaleY:4.6538463
        solids.push_back(rectF(277, 81, 32, 149));
        
        // Bottom left section: x:39, y:199, scaleX:5.2999997, scaleY:1.4230769
        solids.push_back(rectF(39, 199, 170, 46));  // 5.3 * 32 ≈ 170 width, 1.42 * 32 ≈ 46 height
        
        // Bottom right section: x:185, y:200, scaleX:4.7, scaleY:1.3846154
        solids.push_back(rectF(185, 200, 150, 44));  // 4.7 * 32 = 150 width, 1.38 * 32 ≈ 44 height
        
        // Bottom door area (smaller wall): x:128, y:235, scaleX:3.45, scaleY:0.5769231
        // This is BELOW the door opening to prevent walking off screen
        solids.push_back(rectF(128, 235, 110, 18));  // 3.45 * 32 = 110 width
        
        // Computer desk collision
        solids.push_back(deskRect());
        
    } else {
        // Room 2 collisions (unchanged)
        solids.push_back(rectF(0, 0, 1000, 100));
        solids.push_back(rectF(65, 334, 915, 32));
        solids.push_back(wagonRect());
    }
    return solids;
}

bool Game::blocked(const RectF& r, const std::vector<RectF>& solids) const {
    for (const auto& s : solids) if (overlaps(r, s)) return true;
    return false;
}

void Game::moveAndCollide(int id, float dx, float dy, const std::vector<RectF>& solids) {
    if (dx == 0 && dy == 0) return;
    float oldX = players[id].x;
    players[id].x += dx;
    if (blocked(playerBoundingBox(id), solids)) players[id].x = oldX;
    float oldY = players[id].y;
    players[id].y += dy;
    if (blocked(playerBoundingBox(id), solids)) players[id].y = oldY;
}

void Game::updateOverworld() {
    if (warpCooldown > 0) warpCooldown--;
    std::vector<RectF> solids = buildSolids();
    for (int i = 0; i < 2; i++) {
        if (!players[i].active) continue;
        float speed = (players[i].input.btnB) ? (2.2f * currentRoomScale) : (1.4f * currentRoomScale);
        float dx = 0.0f, dy = 0.0f;
        bool moving = false;
        if (players[i].input.left) { dx -= speed; players[i].direction = 2; moving = true; }
        else if (players[i].input.right) { dx += speed; players[i].direction = 3; moving = true; }
        if (players[i].input.up) { dy -= speed; players[i].direction = 1; moving = true; }
        else if (players[i].input.down) { dy += speed; players[i].direction = 0; moving = true; }
        moveAndCollide(i, dx, dy, solids);
        
        // Warp trigger - player 1 only
        if (i == 0 && warpCooldown == 0 && overlaps(playerBoundingBox(0), warpRect())) {
            warpCooldown = 60;
            currentRoom = (currentRoom == ROOM_1) ? ROOM_2 : ROOM_1;
            GRRLIB_FreeTexture(assets.roomTex);
            loadAssets();
            roomX = (640.0f - (assets.roomTex->w * currentRoomScale)) / 2.0f;
            roomY = (480.0f - (assets.roomTex->h * currentRoomScale)) / 2.0f;
            players[0].x = roomX + (155.0f * currentRoomScale);
            players[0].y = (currentRoom == ROOM_1) ? (roomY + 200.0f * currentRoomScale) : (roomY + 140.0f * currentRoomScale);
        }
        
        // Wagon interaction in Room 2
        if (currentRoom == ROOM_2 && i == 0 && players[i].input.btnA && overlaps(facingRect(i), wagonRect())) {
            currentText = "* (An old wagon.)\n* (You feel like you've used it before.)";
            textCharCount = 0; textTick = 0; gameState = STATE_DIALOGUE;
        }
        
        if (moving) {
            int animSpeed = (players[i].input.btnB) ? 5 : 8;
            if (++players[i].tick > animSpeed) { players[i].frame = (players[i].frame + 1) % 4; players[i].tick = 0; }
        } else { players[i].frame = 0; players[i].tick = 0; }
    }
    
    // Camera follow player
    float rW = assets.roomTex->w * currentRoomScale;
    float rH = assets.roomTex->h * currentRoomScale;
    camX = players[0].x - 320.0f;
    camY = players[0].y - 240.0f;
    if (camX < roomX) camX = roomX;
    if (camY < roomY) camY = roomY;
    if (camX > roomX + rW - 640.0f) camX = roomX + rW - 640.0f;
    if (camY > roomY + rH - 480.0f) camY = roomY + rH - 480.0f;
    if (rW < 640.0f) camX = roomX - (640.0f - rW) / 2.0f;
    if (rH < 480.0f) camY = roomY - (480.0f - rH) / 2.0f;
}

void Game::render() {
    GRRLIB_FillScreen(0x000000FF); 
    if (assets.roomTex) GRRLIB_DrawImg(std::round(roomX - camX), std::round(roomY - camY), assets.roomTex, 0, currentRoomScale, currentRoomScale, 0xFFFFFFFF);
    std::vector<Renderable> scene;
    if (currentRoom == ROOM_1 && assets.computerTex) scene.push_back({ assets.computerTex, roomX + (DESK_X * currentRoomScale), roomY + (DESK_Y * currentRoomScale), roomY + (DESK_Y * currentRoomScale) + (14.0f * currentRoomScale), false, 0xFFFFFFFF });
    if (currentRoom == ROOM_2 && assets.wagonTex) scene.push_back({ assets.wagonTex, roomX + (WAGON_X * currentRoomScale), roomY + (WAGON_Y * currentRoomScale), roomY + (WAGON_Y * currentRoomScale) + (12.0f * currentRoomScale), false, 0xFFFFFFFF });
    
    // Mirror reflection code (Room 2 only)
    float mirrorBaseY = roomY + (169.0f + (2.71875f * 32.0f)) * currentRoomScale; 
    float mirrorLeftX = roomX + 174.0f * currentRoomScale;
    float mirrorRightX = mirrorLeftX + (1.125f * 32.0f) * currentRoomScale;
    
    for (int i = 0; i < 2; i++) {
        if (!players[i].active) continue; 
        if (currentRoom == ROOM_1 && players[i].x > mirrorLeftX && players[i].x < mirrorRightX && players[i].y >= mirrorBaseY && players[i].y < mirrorBaseY + (60.0f * currentRoomScale)) {
            GRRLIB_texImg* refTex = nullptr;
            int refDir = (players[i].direction == 0) ? 1 : (players[i].direction == 1 ? 0 : players[i].direction);
            switch (refDir) {
                case 1:  refTex = assets.anim.up[players[i].frame]; break;
                case 2:  refTex = assets.anim.left[players[i].frame]; break;
                case 3:  refTex = assets.anim.right[players[i].frame]; break;
                default: refTex = assets.anim.down[players[i].frame]; break;
            }
            if (refTex) {
                float distFromMirror = players[i].y - mirrorBaseY;
                scene.push_back({ refTex, players[i].x, mirrorBaseY - distFromMirror, mirrorBaseY - 1.0f, true, 0x88888888 });
            }
        }
        GRRLIB_texImg* pTex = nullptr;
        switch (players[i].direction) {
            case 1:  pTex = assets.anim.up[players[i].frame]; break;
            case 2:  pTex = assets.anim.left[players[i].frame]; break;
            case 3:  pTex = assets.anim.right[players[i].frame]; break;
            default: pTex = assets.anim.down[players[i].frame]; break;
        }
        if (pTex) scene.push_back({ pTex, players[i].x, players[i].y, players[i].y, true, players[i].colorTint });
    }
    
    std::sort(scene.begin(), scene.end(), [](const Renderable& a, const Renderable& b) { return a.sortY < b.sortY; });
    for (auto& obj : scene) {
        if (!obj.tex) continue; 
        float drawX = std::round(obj.x - camX - (obj.isPlayer ? (obj.tex->w * currentRoomScale / 2.0f) : 0));
        float drawY = std::round(obj.y - camY - (obj.isPlayer ? (obj.tex->h * currentRoomScale) : 0));
        GRRLIB_DrawImg(drawX, drawY, obj.tex, 0, currentRoomScale, currentRoomScale, obj.color);
    }
    if (gameState == STATE_DIALOGUE) drawDialogueBox(currentText);
    GRRLIB_Render();
}

void Game::drawDialogueBox(std::string text) {
    GRRLIB_Rectangle(50, 320, 540, 120, 0x000000FF, 1);
    GRRLIB_Rectangle(55, 325, 530, 110, 0xFFFFFFFF, 0);
    if (++textTick >= 2) {
        if (textCharCount < (int)text.length()) { textCharCount++; playSound(snd_txttor_raw, snd_txttor_raw_size); }
        textTick = 0;
    }
    std::string visibleText = text.substr(0, textCharCount);
    if (assets.gameFont) {
        size_t newlinePos = visibleText.find('\n');
        if (newlinePos != std::string::npos) {
            GRRLIB_PrintfTTF(75, 345, assets.gameFont, visibleText.substr(0, newlinePos).c_str(), 20, 0x000000FF);
            GRRLIB_PrintfTTF(75, 375, assets.gameFont, visibleText.substr(newlinePos + 1).c_str(), 20, 0x000000FF);
        } else GRRLIB_PrintfTTF(75, 345, assets.gameFont, visibleText.c_str(), 20, 0x000000FF);
    } else GRRLIB_Printf(75, 345, NULL, 1.0f, 0x000000FF, "%s", visibleText.c_str());
}

void Game::update() {
    readInput();
    updateWiimoteStream();
    if (gameState == STATE_OVERWORLD) updateOverworld();
    else if (gameState == STATE_DIALOGUE && players[0].input.btnA) {
        if (textCharCount < (int)currentText.length()) { textCharCount = (int)currentText.length(); textTick = 0; }
        else { gameState = STATE_OVERWORLD; textCharCount = 0; textTick = 0; aSoundPlaying = false; wmAudioData = nullptr; }
    }
    updateFPS();
}

bool Game::checkCollision(float px, float py, float pw, float ph, float ox, float oy, float ow, float oh) { return (px < ox+ow && px+pw > ox && py < oy+oh && py+ph > oy); }

void Game::updateFPS() {
    frameCount++; u32 now = gettime(); u32 elapsed = ticks_to_millisecs(now - lastFPSTime);
    if (elapsed >= 1000) { currentFPS = frameCount; frameCount = 0; lastFPSTime = now; }
}

void Game::unloadAssets() {
    if (assets.roomTex) GRRLIB_FreeTexture(assets.roomTex);
    if (assets.computerTex) GRRLIB_FreeTexture(assets.computerTex);
    if (assets.gameFont) GRRLIB_FreeTTF(assets.gameFont);
    if (assets.wagonTex) GRRLIB_FreeTexture(assets.wagonTex);
    for (int i = 0; i < 4; i++) {
        if (assets.anim.down[i]) GRRLIB_FreeTexture(assets.anim.down[i]);
        if (assets.anim.up[i]) GRRLIB_FreeTexture(assets.anim.up[i]);
        if (assets.anim.left[i]) GRRLIB_FreeTexture(assets.anim.left[i]);
        if (assets.anim.right[i]) GRRLIB_FreeTexture(assets.anim.right[i]);
    }
}