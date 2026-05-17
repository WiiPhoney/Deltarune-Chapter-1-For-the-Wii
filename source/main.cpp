#include <gccore.h>
#include <wiiuse/wpad.h>
#include <grrlib.h>
#include <unistd.h>
#include <asndlib.h>
#include <ogc/lwp_watchdog.h>
#include "game.h"

int main(int argc, char **argv) {
    ASND_Init();
    WPAD_Init();
    GRRLIB_Init();
    WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR);
    WPAD_Rumble(WPAD_CHAN_0, 1); usleep(100000); WPAD_Rumble(WPAD_CHAN_0, 0);
    Game *game = new Game();
    while (1) {
        WPAD_ScanPads();
        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) break;
        game->update();
        game->render();
    }
    delete game;
    GRRLIB_Exit();
    return 0;
}