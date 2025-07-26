#pragma once

#include <SDL3/SDL_log.h>

#define INFO(format, ...) log_generic(src_basename(__FILE__), __LINE__, format, ##__VA_ARGS__)
#define WARN(format, ...) log_generic(src_basename(__FILE__), __LINE__, "! " format, ##__VA_ARGS__)
#define WTF(format, ...) log_generic(src_basename(__FILE__), __LINE__, "!! " format, ##__VA_ARGS__)
#define FATAL(format, ...) log_fatal(src_basename(__FILE__), __LINE__, format, ##__VA_ARGS__)

#ifndef NDEBUG
#define DEBUG(format, ...) log_generic(src_basename(__FILE__), __LINE__, format, ##__VA_ARGS__)
#else
#define DEBUG(format, ...)
#endif

const char* src_basename(const char*);
void log_callback(void*, int, SDL_LogPriority, const char*);
void log_init();
void log_generic(const char*, int, const char*, ...);
void log_script(const char*, const char*, int, const char*, ...);
void log_fatal(const char*, int, const char*, ...);
void log_teardown();
