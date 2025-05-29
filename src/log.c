#include <stdio.h>

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_messagebox.h>
#include <SDL3/SDL_platform.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_time.h>

#include "internal.h"
#include "log.h"
#include "mem.h"

#define LOG_FILENAME "lameo.log"

static SDL_IOStream* log_file = NULL;

const char* src_basename(const char* path) {
    const char* s = SDL_strrchr(path, '/');
    if (s == NULL)
        s = SDL_strrchr(path, '\\');

    return s == NULL ? path : s + 1;
}

void log_callback(void* userdata, int category, SDL_LogPriority priority, const char* message) {
    printf("[SDL %u:%u] %s\n", category, priority, message);
    fflush(stdout);

    if (log_file != NULL)
        SDL_IOprintf(log_file, "[SDL %u:%u] %s\n", category, priority, message);
}

void log_init() {
    SDL_SetLogOutputFunction(log_callback, NULL);
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_TRACE);

    log_file = SDL_IOFromFile(LOG_FILENAME, "w");
    if (log_file == NULL) {
        WARN("Can't open log file: %s", SDL_GetError());
    } else {
        SDL_IOprintf(log_file, "[lameo]\n\n");
        SDL_IOprintf(log_file, "Operating System: %s\n", SDL_GetPlatform());

        SDL_Time ticks;
        SDL_DateTime dt;
        SDL_GetCurrentTime(&ticks);
        SDL_TimeToDateTime(ticks, &dt, false);
        SDL_IOprintf(
            log_file, "Date: %02d/%02d/%04d %02d.%02d.%02d\n\n", dt.day, dt.month, dt.year, dt.hour, dt.minute,
            dt.second
        );
    }

    INFO("Opened");
}

void log_generic(const char* filename, int line, const char* format, ...) {
    printf("[%s:%d] ", filename, line);
    va_list args, args2;
    va_start(args, format);
    va_copy(args2, args);
    vprintf(format, args);
    va_end(args);
    printf("\n");
    fflush(stdout);

    if (log_file != NULL) {
        SDL_IOprintf(log_file, "[%s:%d] ", filename, line);
        SDL_IOvprintf(log_file, format, args2);
        SDL_IOprintf(log_file, "\n");
    }
    va_end(args2);
}

void log_script(const char* source, const char* name, int line, const char* format, ...) {
    printf("{%s:%s:%d} ", source, name, line);
    va_list args, args2;
    va_start(args, format);
    va_copy(args2, args);
    vprintf(format, args);
    va_end(args);
    printf("\n");
    fflush(stdout);

    if (log_file != NULL) {
        SDL_IOprintf(log_file, "{%s:%s:%d} ", source, name, line);
        SDL_IOvprintf(log_file, format, args2);
        SDL_IOprintf(log_file, "\n");
    }
    va_end(args2);
}

void log_fatal(const char* filename, int line, const char* format, ...) {
    char message[256];
    int msgpos = SDL_snprintf(message, sizeof(message), "[%s:%d] !!! ", filename, line);

    va_list args;
    va_start(args, format);
    msgpos += SDL_vsnprintf(message + msgpos, sizeof(message) - msgpos, format, args);
    va_end(args);
    SDL_strlcat(message + msgpos, "\n", sizeof(message) - msgpos);

    printf(message);
    fflush(stdout);

    if (log_file != NULL) {
        SDL_IOprintf(log_file, message);
        SDL_CloseIO(log_file);
        log_file = NULL;
    }

    char dialog[1024];
    SDL_snprintf(dialog, sizeof(dialog), "Fatal error!\n\n%s\nCheck \"%s\" for more details.", message, LOG_FILENAME);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "lameo", dialog, NULL);

    exit(1); // lololololololololololololololololololololololololololololololol
}

void log_teardown() {
    CLOSE_POINTER(log_file, SDL_CloseIO);

    INFO("Closed");
}
