#include "steam.h"
#include "log.h"

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
    ISteamUser* user = caulk_SteamUser();
    return user == NULL ? 0 : caulk_ISteamUser_GetSteamID(user);
}
