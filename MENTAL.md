# Mental notes

Here are my mental notes for lameo's coding conventions. lameo itself may not
follow these 100%, but it's still extremely recommended since it's... you know,
plain C!

## Naming

- Variables and functions use `snake_case`.
- Macros use `UPPER_CASE`, unless they redirect to an internal function prefixed with `_` in which case it's just the name of the function without that prefix.
  - Magic numbers and constants (i.e. fixed sizes) should be macros for customizability.
- Enums are never typedef'd; always use the `enum` prefix and `CamelCase` for the type name.
  - Enum items always have a shortened prefix of their type and use `SCREAMING_SNAKE_CASE`
- Structs are never typedef'd; always use the `struct` prefix and `CamelCase` for the type name.

## Restrictions

- Do **not** directly include the following headers in your code:
    - `std*.h` (use SDL3 instead)
    - `SDL3/SDL_stdinc.h` (use `mem.h` instead)
    - `SDL3/SDL_log.h` (use `log.h` instead)
    - `SDL3/SDL_filesystem.h`, `SDL3/SDL_iostream.h`, or `yyjson.h` (use `file.h` instead)
    - `cglm/cglm.h` (use `math.h` instead)
    - `glad/gl.h`, `SDL3/SDL_video.h` or `SDL3/SDL_opengl.h` (use `video.h` instead)
    - `fmod.h` (use `audio.h` instead)
        - Using **FMOD** directly outside of `audio.h` is discouraged as its functionality is supposed to be abstracted.
    - `lauxlib.h`, `lua.h` or `lualib.h` (use `script.h` instead)
    - `caulk.h` (use `steam.h` instead)
        - Using **caulk** directly outside of `steam.h` is discouraged as its functionality is supposed to be abstracted.
- Only use SDL3 equivalents of C standard library functions and macros **exclusively**.
- Only use `lame_*` functions for memory handling **exclusively**.
    - For shortcutting/convenience, the following destructor macros are provided:
        - `FREE_POINTER(varname)`
        - `CLOSE_POINTER(varname, callback)`
        - `CLOSE_HANDLE(varname, callback)`
    - **EXCEPTION:** Some third-party libraries may allow custom memory allocation, in which case you can directly use SDL3's memory functions.
- Only use the following macros for logging **exclusively**:
    - `INFO(format, ...)`
    - `WARN(format, ...)`
    - `WTF(format, ...)`
    - `FATAL(format, ...)`
    - **EXCEPTION:** Some third-party libraries may allow custom logging callbacks, in which case you can hook those to lameo's logging system using either the logging macros or by defining a custom `log_*` function.
- Only use the following functions for filenames when handling the following categories **exclusively**:
    - Internal files (controller mappings, debugging) -> `get_base_path()`
    - Definitions (mod information, languages) -> `char path[]` in `struct Mod`
    - Assets (images, sounds, JSON) -> `get_mod_file(filename, exclude)`
        - Mods can override assets, so the latest replacement is always returned.
    - User data (saves, extras) -> `get_user_path()`
        - User data is unique for each Steam ID. If Steam is not available, the user ID `0` is used as a fallback for local data.
    - Miscellaneous (settings, controls) -> `get_pref_path()`
        - These are global for every user.
- Only use `SDL_LoadFile(file, datasize)` and `SDL_SaveFile(file, data, datasize)` for file I/O **exclusively**.
    - These functions take an absolute or relative path. Refer to the previous restriction on how the path must be specified.
    - File streaming through `SDL3/SDL_iostream.h` is possible, but generally you should load data as buffers.
- Only use `load_json(filename)` and `save_json(json, filename)` for JSON I/O **exclusively**.
    - These functions take an absolute or relative path. Refer to the previous restriction on how the path must be specified.
    - **EXCEPTION:** You may use `yyjson_read_opts()` to read directly from a buffer, but you must use the `JSON_READ_FLAGS` macro for the `flg` argument.

## Memory Safety

- In some cases such as assets, actors, etc., pointers may be destroyed by other sources and as such may lead to dangling pointers in external code. `Fixture`s and `HandleID`s are built specifically for this occasion, so that you may use handles for pointing instead. Invalid handles will always resolve into `NULL`.
- Do **not** allow lameo to cause segfaults. Use the `FATAL()` macro so you can catch critical errors outside of debugging.
  - **EXCEPTION:** In rare cases, some segfaults may be out of lameo's control (i.e. Steam overlay or an error ).
- Every bit of memory **must** be freed when closing lameo.
  - Some memory leaks may be out of lameo's control. Search for `MEMORY LEAK` in the repository for comments on possible cases.
  - **EXCEPTION:** It's fine to leak memory on a fatal error, as lameo will forcefully exit without freeing anything anyway.
