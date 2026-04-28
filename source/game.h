#ifndef GAME_H
#define GAME_H

#include <grrlib.h>
#include <wiiuse/wpad.h>
#include <vector>
#include <algorithm>
#include <string>
#include <gccore.h>

#define SCALE 1.5f

enum GameState {
    STATE_OVERWORLD,
    STATE_DIALOGUE
};

extern const u8  determination_ttf[];
extern const u32 determination_ttf_size;

extern const u8  room_bg_png[], computer_png[], wagon1_png[];
extern const u8  down1_png[], down2_png[], down3_png[], down4_png[];
extern const u8  left1_png[], left2_png[], left3_png[], left4_png[];
extern const u8  right1_png[], right2_png[], right3_png[], right4_png[];
extern const u8  uo1_png[], uo2_png[], uo3_png[], uo4_png[];

struct RectF {
    float x, y, w, h;
};

struct Renderable {
    GRRLIB_texImg* tex;
    float x, y;
    float sortY;
    bool isPlayer;
    u32 color;
};

struct PlayerInput {
    bool left, right, up, down, btnA, btnB;
};

struct Player {
    bool active;
    float x, y;
    int direction; // 0: Down, 1: Up, 2: Left, 3: Right
    int frame;
    int tick;
    PlayerInput input;
    u32 colorTint;
};

class Game {
public:
    Game();
    ~Game();

    void update();
    void render();

    void playSound(const u8* sound, u32 size);

    void playSoundWiimote(const u8* data, u32 size);
    void updateWiimoteStream();

private:
    void loadAssets();
    void unloadAssets();
    void readInput();
    void updateOverworld();
    void drawDialogueBox(std::string text);

    bool checkCollision(float px, float py, float pw, float ph,
                        float ox, float oy, float ow, float oh);
    RectF playerFeetRect(int id) const;
    RectF playerBoundingBox(int id) const;
    RectF facingRect(int id) const;
    RectF deskRect() const;
    RectF wagonRect() const;
    std::vector<RectF> buildSolids() const;
    bool blocked(const RectF& r, const std::vector<RectF>& solids) const;
    void moveAndCollide(int id, float dx, float dy, const std::vector<RectF>& solids);

    void updateFPS();
    u32 lastFPSTime;
    int frameCount;
    int currentFPS;

    GameState gameState;
    float roomX, roomY;
    std::string currentText;
    int textCharCount;
    int textTick;

    Player players[2];

    bool aSoundPlaying;
    const u8* wmAudioData;
    u32 wmAudioSize;
    u32 wmAudioOffset;

    struct {
        GRRLIB_texImg *roomTex, *computerTex, *wagonTex;
        GRRLIB_ttfFont *gameFont;
        struct {
            GRRLIB_texImg *down[4], *up[4], *left[4], *right[4];
        } anim;
    } assets;
};

#endif