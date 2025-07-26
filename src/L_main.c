#ifdef _MSC_VER
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <SDL3/SDL_main.h>

#include "L_internal.h"

int main(int argc, char** argv) {
#ifdef _MSC_VER
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF);

    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
#endif

    SDL_SetAppMetadata("lameo", "0.1", "com.schwungus.lameo");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_CREATOR_STRING, "Schwungus");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_COPYRIGHT_STRING, "Copyright (c) 2025 Schwungus");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_URL_STRING, "https://schwung.us");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_TYPE_STRING, "game");

    const char *config_path = NULL, *controls_path = NULL;
    bool bypass_shader = false;
    for (int i = 0; i < argc; i++) {
        if (SDL_strcmp(argv[i], "-config") == 0)
            config_path = argv[++i];
        else if (SDL_strcmp(argv[i], "-controls") == 0)
            controls_path = argv[++i];
        else if (SDL_strcmp(argv[i], "-bypass_shader") == 0)
            bypass_shader = true;
    }

    init(config_path, controls_path, bypass_shader);
    loop();
    cleanup();

#ifdef _MSC_VER
    _CrtDumpMemoryLeaks();
#endif

    return 0;
}
