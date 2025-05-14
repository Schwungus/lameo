#pragma once

//#include "log.h"

void* _lame_alloc(size_t, const char*, int);
void _lame_free(void**, const char*, int);
void* _lame_copy(void*, const void*, size_t, const char*, int);
void _lame_realloc(void**, size_t, const char*, int);

#define lame_alloc(size) _lame_alloc(size, __FILE__, __LINE__)
#define lame_free(ptr) _lame_free((void**)ptr, __FILE__, __LINE__)
#define lame_copy(dest, src, size) _lame_copy(dest, src, size, __FILE__, __LINE__)
#define lame_realloc(ptr, size) _lame_realloc((void**)ptr, size, __FILE__, __LINE__)

#define CLOSE_POINTER(varname, callback) if (varname != NULL) {\
    callback(varname);\
    varname = NULL;\
}

#define CLOSE_HANDLE(varname, callback) if (varname != 0) {\
    callback(varname);\
    varname = 0;\
}

// A database for storing handles (numeric IDs -> pointer or NULL).
struct Fixture {
    struct Handle* handles;
    size_t size, capacity; // Size = live handles, capacity = array size.
    size_t next; // Next invalid handle in the array.
    size_t first, last; // First and last valid handles in the array.
};

struct Handle {
    void* ptr;
    uint16_t generation;
};

typedef uint32_t HandleID;
#define HANDLE_LIMIT 0xFFFF

struct Fixture* create_fixture();
void destroy_fixture(struct Fixture*);

HandleID create_handle(struct Fixture*, void*);
void destroy_handle(struct Fixture*, HandleID);
struct Handle* hid_to_handle(struct Fixture*, HandleID);
void* hid_to_pointer(struct Fixture*, HandleID);
