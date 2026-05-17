#ifndef GAME_H
#define GAME_H

#include <grrlib.h>

// GRRLIB defines macros like R(c), G(c), B(c), A(c)
// These can break user code if you use the same identifiers (like R(x,y,w,h))
#ifdef R
#undef R
#endif

#include <wiiuse/wpad.h>
#include <vector>
#include <algorithm>
#include <string>
#include <gccore.h>

#define SCALE 2.0f

enum GameState {
    STATE_OVERWORLD,
    STATE_DIALOGUE
};

enum RoomID {
    ROOM_1,
    ROOM_2
};

extern const u8  determination_ttf[];
extern const u32 determination_ttf_size;

extern const u8  room_bg_png[];
extern const u8  room_bg2_png[];

extern const u8  computer_png[];
extern const u8  wagon1_png[];

extern const u8  down1_png[];
extern const u8  down2_png[];
extern const u8  down3_png[];
extern const u8  down4_png[];

extern const u8  left1_png[];
extern const u8  left2_png[];
extern const u8  left3_png[];
extern const u8  left4_png[];

extern const u8  right1_png[];
extern const u8  right2_png[];
extern const u8  right3_png[];
extern const u8  right4_png[];

extern const u8  uo1_png[];
extern const u8  uo2_png[];
extern const u8  uo3_png[];
extern const u8  uo4_png[];

extern const u8  snd_txttor_raw[];
extern const u32 snd_txttor_raw_size;

struct RectF {
    float x;
    float y;
    float w;
    float h;
};

struct Renderable {
    GRRLIB_texImg* tex;

    float x;
    float y;

    float sortY;

    bool isPlayer;

    u32 color;
};

struct PlayerInput {
    bool left;
    bool right;
    bool up;
    bool down;

    bool btnA;
    bool btnB;
};

struct Player {
    bool active;

    float x;
    float y;

    int direction;
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

    bool checkCollision(
        float px,
        float py,
        float pw,
        float ph,
        float ox,
        float oy,
        float ow,
        float oh
    );

    RectF playerFeetRect(int id) const;
    RectF playerBoundingBox(int id) const;
    RectF facingRect(int id) const;

    RectF deskRect() const;
    RectF wagonRect() const;
    RectF warpRect() const;

    std::vector<RectF> buildSolids() const;
    std::vector<RectF> buildRoom1Solids() const;
    std::vector<RectF> buildRoom2Solids() const;

    bool blocked(
        const RectF& r,
        const std::vector<RectF>& solids
    ) const;

    void moveAndCollide(
        int id,
        float dx,
        float dy,
        const std::vector<RectF>& solids
    );

    void updateFPS();

private:

    u32 lastFPSTime;
    u32 frameCount;
    u32 currentFPS;

    GameState gameState;

    RoomID currentRoom;

    float roomX;
    float roomY;

    float currentRoomScale;

    std::string currentText;

    int textCharCount;
    int textTick;

    Player players[2];

    bool aSoundPlaying;

    const u8* wmAudioData;

    u32 wmAudioSize;
    u32 wmAudioOffset;

    int warpCooldown;
    float camX;
    float camY;

    struct {

        GRRLIB_texImg* roomTex;
        GRRLIB_texImg* computerTex;
        GRRLIB_texImg* wagonTex;

        GRRLIB_ttfFont* gameFont;

        struct {
            GRRLIB_texImg* down[4];
            GRRLIB_texImg* up[4];
            GRRLIB_texImg* left[4];
            GRRLIB_texImg* right[4];
        } anim;

    } assets;
};

#endif