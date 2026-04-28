#include <gccore.h>
#include <wiiuse/wpad.h>
#include <grrlib.h>
#include <unistd.h>
#include <ogc/lwp_watchdog.h>   #include "game.h"

int main(int argc, char **argv) {

    VIDEO_Init();
    WPAD_Init();
    GRRLIB_Init();

    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();

    WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR);

    WPAD_Rumble(WPAD_CHAN_0, 1);
    usleep(100000);
    WPAD_Rumble(WPAD_CHAN_0, 0);

    gettime();

    Game game;

    while (1) {

        WPAD_ScanPads();

        game.update();

        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)
            break;

        game.render();

        VIDEO_WaitVSync();
    }

    GRRLIB_Exit();
    return 0;
}