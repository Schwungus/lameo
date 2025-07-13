#pragma once

#include <SDL3/SDL_stdinc.h>

void* _lame_alloc(size_t, const char*, int);
void _lame_free(void**, const char*, int);
void* _lame_copy(void*, const void*, size_t, const char*, int);
void _lame_realloc(void**, size_t, const char*, int);
void _lame_set(void*, char, size_t, const char*, int);

#define lame_alloc(size) _lame_alloc(size, __FILE__, __LINE__)
#define lame_free(ptr) _lame_free((void**)ptr, __FILE__, __LINE__)
#define lame_copy(dest, src, size) _lame_copy(dest, src, size, __FILE__, __LINE__)
#define lame_realloc(ptr, size) _lame_realloc((void**)ptr, size, __FILE__, __LINE__)
#define lame_set(dest, val, size) _lame_set(dest, val, size, __FILE__, __LINE__)

#define FREE_POINTER(varname)                                                                                          \
    if (varname != NULL)                                                                                               \
        lame_free(&varname);

#define CLOSE_POINTER(varname, callback)                                                                               \
    if (varname != NULL) {                                                                                             \
        callback(varname);                                                                                             \
        varname = NULL;                                                                                                \
    }

#define CLOSE_HANDLE(varname, callback)                                                                                \
    if (varname != 0) {                                                                                                \
        callback(varname);                                                                                             \
        varname = 0;                                                                                                   \
    }

uint8_t read_u8(uint8_t**);
uint16_t _read_u16(uint8_t**, const char*, int);
uint32_t _read_u32(uint8_t**, const char*, int);
uint64_t _read_u64(uint8_t**, const char*, int);
int8_t read_s8(uint8_t**);
int16_t _read_s16(uint8_t**, const char*, int);
int32_t _read_s32(uint8_t**, const char*, int);
int64_t _read_s64(uint8_t**, const char*, int);
float _read_f32(uint8_t**, const char*, int);

#define read_u16(buf) _read_u16(buf, __FILE__, __LINE__)
#define read_u32(buf) _read_u32(buf, __FILE__, __LINE__)
#define read_u64(buf) _read_u64(buf, __FILE__, __LINE__)
#define read_s16(buf) _read_s16(buf, __FILE__, __LINE__)
#define read_s32(buf) _read_s32(buf, __FILE__, __LINE__)
#define read_s64(buf) _read_s64(buf, __FILE__, __LINE__)
#define read_f32(buf) _read_f32(buf, __FILE__, __LINE__)

/*
   This is a handle system I made on a whim. It's called "bumbling smartass".

   Fixtures are databases containing local Handles to pointers.
   Handles reduce dangling pointer problems and make it easy to tell when
   something allegedly exists -> everyone is happy.

   By default, HandleIDs are 32-bit integers; first 16 bits is index, the rest
   is generation. So even though Fixture capacity grows dynamically, the hard
   limit to Handle amount per fixture is ~65535.

   The smallest invalid Handle index always gets recycled. That's why Handles
   have incrementing generations on create; that way you still get NULL on
   stale HandleIDs. Generations always start at 1, then wrap after 65535.
   So unless you create and destroy a godless amount of Handles (around 65k to
   4.2b), there's no chance of a stale HandleID working again.

   If a generation wraparound does happen though, you'll at least get a warning
   in the log.
*/
struct Fixture {
    struct Handle* handles;
    size_t size, capacity; // Size = live handles, capacity = array size.
    size_t next;           // Next invalid handle in the array.
};

struct Handle {
    void* ptr;
    uint16_t generation;
};

#define HID_TYPE uint32_t
#define HID_HALF uint16_t
#define HID_BITS 16
#define HID_LIMIT 0xFFFF

typedef HID_TYPE HandleID;

struct Fixture* _create_fixture(const char*, int);
void _destroy_fixture(struct Fixture*, const char*, int);
HandleID _create_handle(struct Fixture*, void*, const char*, int);
void _destroy_handle(struct Fixture*, HandleID, const char*, int);

#define create_fixture() _create_fixture(__FILE__, __LINE__)
#define destroy_fixture(fixture) _destroy_fixture(fixture, __FILE__, __LINE__)
#define create_handle(fixture, ptr) _create_handle(fixture, (void*)ptr, __FILE__, __LINE__)
#define destroy_handle(fixture, hid) _destroy_handle(fixture, (HandleID)hid, __FILE__, __LINE__)

struct Handle* hid_to_handle(struct Fixture*, HandleID);
void* hid_to_pointer(struct Fixture*, HandleID);

// Hash maps
struct KeyValuePair {
    char* key;
    void* value;
};

struct HashMap {
    struct KeyValuePair* items;
    size_t count, capacity;
};

struct HashMap* _create_hash_map(const char*, int);
void _destroy_hash_map(struct HashMap*, bool, const char*, int);
bool _to_hash_map(struct HashMap*, const char*, void*, bool, const char*, int);
void* from_hash_map(struct HashMap*, const char*);
void* _pop_hash_map(struct HashMap*, const char*, bool, const char*, int);

#define create_hash_map() _create_hash_map(__FILE__, __LINE__)
#define destroy_hash_map(map, nuke) _destroy_hash_map(map, nuke, __FILE__, __LINE__)
#define to_hash_map(map, key, value, nuke) _to_hash_map(map, key, value, nuke, __FILE__, __LINE__)
// #define from_hash_map(map, key) _from_hash_map(map, key, __FILE__, __LINE__)
#define pop_hash_map(map, key, nuke) _pop_hash_map(map, key, nuke, __FILE__, __LINE__)

struct IKeyValuePair {
    bool occupied;
    uint32_t key;
    void* value;
};

struct IntMap {
    struct IKeyValuePair* items;
    size_t count, capacity;
};

struct IntMap* _create_int_map(const char*, int);
void _destroy_int_map(struct IntMap*, bool, const char*, int);
bool _to_int_map(struct IntMap*, uint32_t, void*, bool, const char*, int);
void* from_int_map(struct IntMap*, uint32_t);
void* pop_int_map(struct IntMap*, uint32_t, bool, const char*, int);

#define create_int_map() _create_int_map(__FILE__, __LINE__)
#define destroy_int_map(map, nuke) _destroy_int_map(map, nuke, __FILE__, __LINE__)
#define to_int_map(map, key, value, nuke) _to_int_map(map, key, value, nuke, __FILE__, __LINE__)
// #define from_int_map(map, key) _from_int_map(map, key, __FILE__, __LINE__)
#define pop_int_map(map, key, nuke) _pop_int_map(map, key, nuke, __FILE__, __LINE__)
