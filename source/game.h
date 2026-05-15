#ifndef GAME_H
#define GAME_H

#include <vector>
#include <string>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <grrlib.h>

struct RectF {
    float x, y, w, h;
};

struct InputState {
    bool up, down, left, right, btnA, btnB;
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

struct Renderable {
    GRRLIB_texImg* tex;
    float x, y, sortY;
    bool isPlayer;
    u32 color;
};

enum GameState {
    STATE_OVERWORLD,
    STATE_DIALOGUE
};

static constexpr float SCALE = 2.0f;

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
    
    void moveAndCollide(int id, float dx, float dy, const std::vector<RectF>& solids);
    void buildSolids(std::vector<RectF>& solids) const;
    std::vector<RectF> buildSolids() const;
    
    void updateWiimoteStream();
    void playSoundWiimote(const u8* data, u32 size);
    void playSound(const u8* sound, u32 size);
    void updateFPS();
    void drawDialogueBox(std::string text);

    RectF playerBoundingBox(int id) const;
    RectF playerFeetRect(int id) const;
    RectF facingRect(int id) const;
    RectF wagonRect() const;
    RectF deskRect() const;
    bool blocked(const RectF& r, const std::vector<RectF>& solids) const;
    bool checkCollision(float px, float py, float pw, float ph, float ox, float oy, float ow, float oh);

    struct {
        GRRLIB_texImg *roomTex, *computerTex, *wagonTex;
        GRRLIB_ttfFont *gameFont;
        struct {
            GRRLIB_texImg *down[4], *up[4], *left[4], *right[4];
        } anim;
    } assets;

    std::vector<Renderable> sceneBuffer;
    std::vector<RectF> solidsCache;
    Player players[2];
    GameState gameState;
    std::string currentText;
    int textCharCount, textTick;
    float roomX, roomY;

    bool aSoundPlaying;
    const u8* wmAudioData;
    u32 wmAudioSize, wmAudioOffset;

    u32 lastFPSTime, frameCount, currentFPS;
};

#endif 