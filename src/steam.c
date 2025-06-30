#include "steam.h"
#include "log.h"

static bool on_steam = false;

void steam_init() {
    if (!(on_steam = caulk_Init()))
        WARN("Failed to initialize Steam");

    INFO("Opened");
}

void steam_teardown() {
    caulk_Shutdown();

    INFO("Closed");
}

CSteamID get_steam_id() {
    return on_steam ? caulk_ISteamUser_GetSteamID(caulk_SteamUser()) : 0;
}
