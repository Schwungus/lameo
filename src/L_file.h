#pragma once

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>
#include <yyjson.h>

#include "L_log.h" // IWYU pragma: keep

#define FILE_NAME_MAX 128
#define FILE_PATH_MAX 256

void file_init();
void file_teardown();

const char* get_base_path(const char*);
const char* get_pref_path(const char*);
const char* get_user_path(const char*);
bool is_base_path(const char*);
bool is_pref_path(const char*);
bool is_user_path(const char*);

#define get_filename src_basename
const char* get_file_ext(const char*);

#define JSON_READ_FLAGS (YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS)
#define JSON_WRITE_FLAGS (YYJSON_WRITE_PRETTY | YYJSON_WRITE_NEWLINE_AT_END)

yyjson_doc* load_json(const char*);
const char* save_json(yyjson_mut_doc*, const char*);
