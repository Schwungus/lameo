cmake_minimum_required(VERSION 3.27.0 FATAL_ERROR)

project(
    lameo
    VERSION 0.1.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Libraries
set(BUILD_SHARED_LIBS OFF)
set(BUILD_STATIC_LIBS ON)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# SDL3 (3.2.16)
set(SDL_SHARED OFF)
set(SDL_STATIC ON)
set(SDL_TEST_LIBRARY OFF)
set(SDL_RENDER OFF)
set(SDL_AUDIO OFF)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.16
)
FetchContent_MakeAvailable(SDL3)

# SDL_image (3.2.4)
set(SDLIMAGE_AVIF OFF)
set(SDLIMAGE_TIF OFF)
set(SDLIMAGE_WEBP OFF)

FetchContent_Declare(
    SDL3_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG release-3.2.4
)
FetchContent_MakeAvailable(SDL3_image)

# OpenGL
find_package(OpenGL REQUIRED)

# GLAD (2.0.8, OpenGL 3.3 Core)
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG        v2.0.8
    SOURCE_SUBDIR  cmake
)
FetchContent_MakeAvailable(glad)
glad_add_library(glad_gl_core_33 STATIC API gl:core=3.3)

# cglm
set(CGLM_SHARED OFF)
set(CGLM_STATIC ON)

FetchContent_Declare(
    cglm
    GIT_REPOSITORY https://github.com/recp/cglm.git
    GIT_TAG v0.9.6
)
FetchContent_MakeAvailable(cglm)

# FMOD Core API (2.03.08)
add_subdirectory(${CMAKE_SOURCE_DIR}/include/fmod)

# yyjson (0.11.1)
FetchContent_Declare(
    yyjson
    GIT_REPOSITORY https://github.com/ibireme/yyjson.git
    GIT_TAG 0.11.1
)
FetchContent_MakeAvailable(yyjson)

# Lua (5.4.7)
set(LUA_ENABLE_SHARED OFF)
set(LUA_ENABLE_TESTING OFF)

FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/walterschell/Lua.git
    GIT_TAG 3ed55a56eaa05c9221f40b3c07d0e908eb1067b0
)
FetchContent_MakeAvailable(lua)

# caulk (01.07.2025)
set(STEAMWORKS_SDK_ZIP ${CMAKE_SOURCE_DIR}/include/steam/steamworks_sdk_162.zip)
set(STEAM_APPID ${CMAKE_SOURCE_DIR}/include/steam/steam_appid.txt)
set(CAULK_BUILD_TEST OFF)

FetchContent_Declare(
    caulk
    GIT_REPOSITORY https://github.com/Schwungus/caulk.git
    GIT_TAG 24994950d69e367ed4e5a8333e3276fe9f58197d
)
FetchContent_MakeAvailable(caulk)

set(LIBS SDL3::SDL3-static SDL3_image::SDL3_image-static ${OPENGL_gl_LIBRARY} glad_gl_core_33 cglm fmod yyjson lua_static caulk)

# Build
set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
file(GLOB_RECURSE SOURCES ${SOURCE_DIR}/*.c)
file(GLOB_RECURSE HEADERS ${SOURCE_DIR}/*.h)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PRIVATE ${SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBS})
target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS=1)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND_EXPAND_LISTS
)

# Temporary fix to include Steam API DLLs
set(STEAM_SOURCE_DIR ${CMAKE_SOURCE_DIR}/include/steam)
file(ARCHIVE_EXTRACT INPUT ${STEAMWORKS_SDK_ZIP} DESTINATION ${STEAM_SOURCE_DIR})
set(STEAM_BINARY_DIR ${CMAKE_SOURCE_DIR}/include/steam/sdk/redistributable_bin)

if(WIN32)
    if(${CMAKE_SIZEOF_VOID_P} STREQUAL 4)
        set(STEAM_LIB "steam_api")
    else()
        set(STEAM_LIB "win64/steam_api64")
    endif()
    set(STEAM_DLL ${STEAM_BINARY_DIR}/${STEAM_LIB}.dll)
elseif(LINUX)
    if(${CMAKE_SIZEOF_VOID_P} STREQUAL 4)
        set(STEAM_LIB "linux32")
    else()
        set(STEAM_LIB "linux64")
    endif()
    set(STEAM_DLL ${STEAM_BINARY_DIR}/${STEAM_LIB}/libsteam_api.so)
endif()

if (DEFINED STEAM_DLL)
    message(STATUS "Steam API library at ${STEAM_DLL}")
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} ARGS -E copy ${STEAM_DLL} $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()

# Download controller database
set(GCDB_SOURCE_DIR ${CMAKE_SOURCE_DIR}/include/gamecontrollerdb.txt)
file(DOWNLOAD "https://github.com/mdqinc/SDL_GameControllerDB/raw/refs/heads/master/gamecontrollerdb.txt" ${GCDB_SOURCE_DIR})

# Copy assets
set(MOD_SOURCE_DIR ${CMAKE_SOURCE_DIR}/modsrc)
add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${GCDB_SOURCE_DIR} $<TARGET_FILE_DIR:${PROJECT_NAME}>/gamecontrollerdb.txt
    COMMAND ${CMAKE_COMMAND} -E remove_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/data
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${MOD_SOURCE_DIR} $<TARGET_FILE_DIR:${PROJECT_NAME}>/data
    COMMAND ${CMAKE_COMMAND} -E copy ${STEAM_SOURCE_DIR}/steam_appid.txt $<TARGET_FILE_DIR:${PROJECT_NAME}>/steam_appid.txt
)
