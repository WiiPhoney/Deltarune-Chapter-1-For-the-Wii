#ifndef GAME_H
#define GAME_H

#include <grrlib.h>
#include <wiiuse/wpad.h>
#include <string>
#include <vector>

static constexpr float SCALE = 2.0f;

enum GameState {
    STATE_OVERWORLD,
    STATE_DIALOGUE
};

struct RectF {
    float x, y, w, h;
};

struct InputState {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool btnA = false;
    bool btnB = false;
};

struct Player {
    bool active;
    float x, y;
    int direction;
    int frame;
    int tick;
    u32 colorTint;
    InputState input;
};

struct AssetBundle {
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
};

struct Renderable {
    GRRLIB_texImg* tex;
    float x, y, sortY;
    bool isPlayer;
    u32 color;
};

class Game {
public:
    Game();
    ~Game();

    void update();
    void render();

private:
    void loadAssets();
    void unloadAssets();
    void readInput();
    void updateOverworld();
    void drawDialogueBox(std::string text);
    
    void playSound(const u8* sound, u32 size);
    void playSoundWiimote(const u8* data, u32 size);
    void updateWiimoteStream();

    RectF playerFeetRect(int id) const;
    RectF playerBoundingBox(int id) const;
    RectF facingRect(int id) const;
    RectF deskRect() const;
    RectF wagonRect() const;
    RectF warpRect() const;

    std::vector<RectF> buildSolids() const;
    bool blocked(const RectF& r, const std::vector<RectF>& solids) const;
    void moveAndCollide(int id, float dx, float dy, const std::vector<RectF>& solids);
    bool checkCollision(float px, float py, float pw, float ph, float ox, float oy, float ow, float oh);

    void updateFPS();

    AssetBundle assets;
    Player players[2];
    GameState gameState;
    
    std::string currentText;
    int textCharCount;
    int textTick;

    float roomX, roomY;

    u32 frameCount;
    u32 lastFPSTime;
    u32 currentFPS;

    bool aSoundPlaying;
    const u8* wmAudioData;
    u32 wmAudioSize;
    u32 wmAudioOffset;
};

#endif