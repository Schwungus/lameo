#pragma once

#define INFO(format, ...) log_generic(src_basename(__FILE__), __LINE__, format, ##__VA_ARGS__)
#define WARN(format, ...) log_generic(src_basename(__FILE__), __LINE__, "! " format, ##__VA_ARGS__)
#define ERROR(format, ...) log_generic(src_basename(__FILE__), __LINE__, "!! " format, ##__VA_ARGS__)
#define FATAL(format, ...) log_fatal(src_basename(__FILE__), __LINE__, format, ##__VA_ARGS__)

const char* src_basename(const char*);
void log_init();
void log_generic(const char*, int, const char*, ...);
void log_fatal(const char*, int, const char*, ...);
void log_teardown();
