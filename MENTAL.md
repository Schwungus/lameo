# Mental notes

Here are my mental notes for lameo's coding conventions. lameo itself may not
follow these 100%, but it's still extremely recommended since it's... you know,
plain C!

## Naming

-   Variables and functions use `snake_case`.
-   Macros use `UPPER_CASE`, unless they redirect to an internal function prefixed with `_` in which case it's just a non-prefixed version of the function's name.
    -   Magic numbers and constants (i.e. fixed sizes) should be macros for customizability.
-   Enums are never typedef'd; always use the `enum CamelCase` format.
    -   Enum items always use a shortened prefix of their type and `UPPER_CASE`.
-   Structs are never typedef'd; always use the `struct CamelCase` format.

## Restrictions

-   Do **not** directly include the following headers in your code:
    -   `std*.h` (use SDL3 instead)
    -   `SDL3/SDL_stdinc.h` (use `L_memory.h` instead)
    -   `SDL3/SDL_log.h` (use `L_log.h` instead)
    -   `SDL3/SDL_filesystem.h`, `SDL3/SDL_iostream.h`, or `yyjson.h` (use `L_file.h` instead)
    -   `cglm/cglm.h` (use `L_math.h` instead)
    -   `glad/gl.h`, `SDL3/SDL_video.h` or `SDL3/SDL_opengl.h` (use `L_video.h` instead)
    -   `fmod.h` (use `L_audio.h` instead)
        -   Using **FMOD** directly outside of `L_audio.h` is discouraged as its functionality is supposed to be abstracted.
    -   `lauxlib.h`, `lua.h` or `lualib.h` (use `L_script.h` instead)
    -   `caulk.h` (use `L_steam.h` instead)
        -   Using **caulk** directly outside of `L_steam.h` is discouraged as its functionality is supposed to be abstracted.
-   Only use SDL3 equivalents of C standard library functions and macros **exclusively**.
-   Only use `lame_*` functions for memory handling **exclusively**.
    -   For shortcutting/convenience, the following destructor macros are provided:
        -   `FREE_POINTER()`
        -   `CLOSE_POINTER()`
        -   `CLOSE_HANDLE()`
    -   **EXCEPTION:** Some third-party libraries may allow custom memory allocation, in which case you can directly use SDL3's memory functions.
-   Only use the following macros for logging **exclusively**:
    -   `INFO()`
    -   `WARN()`
    -   `WTF()`
    -   `FATAL()`
    -   **EXCEPTION:** Some third-party libraries may allow custom logging callbacks, in which case you can hook those to lameo's logging system using either the logging macros or by defining a custom `log_*` function.
-   Only use the following functions for filenames when handling the following categories **exclusively**:
    -   Internal files (controller mappings, debugging) -> `get_base_path()`
    -   Definitions (mod information, languages) -> `path` from `struct Mod`
    -   Assets (images, sounds, JSON) -> `get_mod_file()`
        -   Mods can override assets, so the latest replacement is always returned.
    -   User data (saves, extras) -> `get_user_path()`
        -   User data is unique for each Steam ID. If Steam is not available, the user ID `0` is used as a fallback for local data.
    -   Miscellaneous (settings, controls) -> `get_pref_path()`
        -   These are global for every user.
-   Only use `SDL_LoadFile()` and `SDL_SaveFile()` for file I/O **exclusively**.
    -   These functions take an absolute or relative path. Refer to the previous restriction on how the path must be specified.
    -   File streaming through `SDL3/SDL_iostream.h` is possible, but generally you should load data as buffers.
-   Only use `load_json()` and `save_json()` for JSON I/O **exclusively**.
    -   These functions take an absolute or relative path. Refer to the previous restriction on how the path must be specified.
    -   **EXCEPTION:** You may use `yyjson_read_opts()` to read directly from a buffer, but you must use the `JSON_READ_FLAGS` macro for the `flg` argument.

## Safety

-   In some cases such as assets, actors, etc., pointers may be destroyed by other sources and as such may lead to dangling pointers in external code. `Fixture`s and `HandleID`s are built **specifically** for this situation, so that you may use handles for safer pointing. Invalid handles will always resolve into `NULL`. Here is a list of common types with handles:
    | Type | Handle Type | Fixture |
    | ---- | ----------- | ------- |
    | `void*` | `HandleID` | |
    | `struct Texture` | `TextureID` | `texture_handles` |
    | `struct Material` | `MaterialID` | `material_handles` |
    | `struct Model` | `ModelID` | `model_handles` |
    | `struct Animation` | `AnimationID` | `animation_handles` |
    | `struct Font` | `FontID` | `font_handles` |
    | `struct Sound` | `SoundID` | `sound_handles` |
    | `struct Track` | `TrackID` | `track_handles` |
    | `struct Actor` | `HandleID` | `actor_handles` |
    | `struct UI` | `HandleID` | `ui_handles` |
-   Do **not** allow lameo to cause segfaults. Make use of the `FATAL()` macro so you can catch fatal errors outside of debugging.
    -   In sanity checks/assertions, it's recommended to end the fatal error message with a `?` to indicate that the error occured during a sanity check/assertion.
    -   **EXCEPTION:** In rare cases, some segfaults may be out of lameo's control (i.e. Steam overlay injection).
-   All allocated memory **must** be freed when closing lameo.
    -   Some memory leaks may be out of lameo's control. Search for `MEMORY LEAK` in the repository for comments on possible cases.
    -   **EXCEPTION:** It's fine to leak memory on a fatal error, as lameo will forcefully exit without freeing anything anyway.
