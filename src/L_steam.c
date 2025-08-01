#include "L_steam.h"
#include "L_log.h"

void steam_init() {
    if (!caulk_Init())
        WARN("Failed to initialize Steam");

    INFO("Opened");
}

void steam_update() {}

void steam_teardown() {
    caulk_Shutdown();

    INFO("Closed");
}

CSteamID get_steam_id() {
    return (caulk_SteamUser() != NULL) ? caulk_SteamUser_GetSteamID() : 0;
}
