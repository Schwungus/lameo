#include <SDL3/SDL_endian.h>

#include "L_log.h"
#include "L_memory.h"

void* _lame_alloc(size_t size, const char* filename, int line) {
    if (!size)
        log_fatal(src_basename(filename), line, "Allocating 0 bytes?");

    void* ptr = SDL_malloc(size);
    if (ptr == NULL)
        log_fatal(src_basename(filename), line, "Allocation failed");
    return ptr;
}

void _lame_free(void** ptr, const char* filename, int line) {
    if (ptr == NULL || *ptr == NULL)
        log_fatal(src_basename(filename), line, "Freeing a null pointer?");
    SDL_free(*ptr);
    *ptr = NULL;
}

void* _lame_copy(void* dest, const void* src, size_t size, const char* filename, int line) {
    if (dest == NULL || src == NULL)
        log_fatal(src_basename(filename), line, "Copying from %u to %u?", src, dest);
    return SDL_memcpy(dest, src, size);
}

void _lame_realloc(void** ptr, size_t size, const char* filename, int line) {
    if (ptr == NULL || *ptr == NULL)
        log_fatal(src_basename(filename), line, "Resizing a null pointer?");
    if (size <= 0)
        log_fatal(src_basename(filename), line, "Reallocating to 0 bytes?");

    *ptr = SDL_realloc(*ptr, size);
    if (*ptr == NULL)
        log_fatal(src_basename(filename), line, "Reallocation failed");
}

void _lame_set(void* dest, char val, size_t size, const char* filename, int line) {
    if (dest == NULL)
        log_fatal(src_basename(filename), line, "Filling a null pointer?");
    if (size <= 0)
        log_fatal(src_basename(filename), line, "Filling 0 bytes?");
    SDL_memset(dest, val, size);
}

uint8_t read_u8(uint8_t** buf) {
    uint8_t result = **buf;
    *buf += sizeof(uint8_t);
    return result;
}

uint16_t _read_u16(uint8_t** buf, const char* filename, int line) {
    uint16_t result;
    _lame_copy(&result, *buf, sizeof(uint16_t), filename, line);
    *buf += sizeof(uint16_t);
    return SDL_Swap16LE(result);
}

uint32_t _read_u32(uint8_t** buf, const char* filename, int line) {
    uint32_t result;
    _lame_copy(&result, *buf, sizeof(uint32_t), filename, line);
    *buf += sizeof(uint32_t);
    return SDL_Swap32LE(result);
}

uint64_t _read_u64(uint8_t** buf, const char* filename, int line) {
    uint64_t result;
    _lame_copy(&result, *buf, sizeof(uint64_t), filename, line);
    *buf += sizeof(uint64_t);
    return SDL_Swap64LE(result);
}

int8_t read_s8(uint8_t** buf) {
    int8_t result = *(int8_t*)(*buf);
    *buf += sizeof(int8_t);
    return result;
}

int16_t _read_s16(uint8_t** buf, const char* filename, int line) {
    int16_t result;
    _lame_copy(&result, *buf, sizeof(int16_t), filename, line);
    *buf += sizeof(int16_t);
    return SDL_Swap16LE(result);
}

int32_t _read_s32(uint8_t** buf, const char* filename, int line) {
    int32_t result;
    _lame_copy(&result, *buf, sizeof(int32_t), filename, line);
    *buf += sizeof(int32_t);
    return SDL_Swap32LE(result);
}

int64_t _read_s64(uint8_t** buf, const char* filename, int line) {
    int64_t result;
    _lame_copy(&result, *buf, sizeof(int64_t), filename, line);
    *buf += sizeof(int64_t);
    return SDL_Swap64LE(result);
}

float _read_f32(uint8_t** buf, const char* filename, int line) {
    float result;
    _lame_copy(&result, *buf, sizeof(float), filename, line);
    *buf += sizeof(float);
    return SDL_SwapFloatLE(result);
}

double _read_f64(uint8_t** buf, const char* filename, int line) {
    double result;
    _lame_copy(&result, *buf, sizeof(double), filename, line);
    *buf += sizeof(double);
    return SDL_Swap64LE(result);
}

char* read_string(uint8_t** buf) {
    uint8_t* cursor = *buf;
    while (read_u8(&cursor) != '\0') {}
    char* str = SDL_strndup((const char*)(*buf), (cursor - *buf) - 1);
    *buf = cursor;

    return str;
}

struct Fixture* _create_fixture(const char* filename, int line) {
    struct Fixture* fixture = _lame_alloc(sizeof(struct Fixture), filename, line);
    fixture->handles = _lame_alloc(sizeof(struct Handle), filename, line);
    fixture->handles[0].ptr = NULL;
    fixture->handles[0].generation = 0;
    fixture->size = 0;
    fixture->capacity = 1;
    fixture->next = 0;

    // log_generic(src_basename(filename), line, "Created fixture %u", fixture);
    return fixture;
}

void _destroy_fixture(struct Fixture* fixture, const char* filename, int line) {
    // log_generic(src_basename(filename), line, "Destroyed fixture %u", fixture);
    if (fixture->handles != NULL)
        _lame_free((void**)&fixture->handles, filename, line);
    _lame_free((void**)&fixture, filename, line);
}

HandleID _create_handle(struct Fixture* fixture, void* ptr, const char* filename, int line) {
    if (ptr == NULL) {
        log_generic(src_basename(filename), line, "!! Creating a handle for a null pointer?");
        return 0;
    }
    if (fixture->size >= HID_LIMIT)
        log_fatal(src_basename(filename), line, "!!! Out of handles (%u >= %u)", fixture->size, HID_LIMIT);

    // Expand handles array on demand
    size_t old_capacity = fixture->capacity;
    if (fixture->next >= old_capacity) {
        size_t new_capacity = fixture->capacity * 2;
        if (new_capacity <= old_capacity)
            log_fatal(
                src_basename(filename), line, "!!! Out of handle capacity (%u <= %u)", new_capacity, old_capacity
            );

        _lame_realloc((void**)&fixture->handles, new_capacity * sizeof(struct Handle), filename, line);
        fixture->capacity = new_capacity;

        for (int i = old_capacity; i < new_capacity; i++) {
            fixture->handles[i].ptr = NULL;
            fixture->handles[i].generation = 0;
        }
    }

    // (Re)occupy an invalid handle
    size_t index = fixture->next;
    struct Handle* handle = &fixture->handles[index];
    handle->ptr = ptr;
    if (++handle->generation <= 0) {
        log_generic(
            src_basename(filename), line,
            "! %u handle index %u generation wrapped, expect undefined behavior from stale handle IDs", fixture, index
        );
        handle->generation = 1;
    }
    ++fixture->size;

    // Find next invalid handle
    while (1) {
        ++fixture->next;
        if (fixture->next >= fixture->capacity || fixture->handles[fixture->next].ptr == NULL)
            break;
    }

    // Generate handle ID
    HandleID hid = ((HID_HALF)index << HID_BITS) | (HID_HALF)handle->generation;
    // log_generic(
    //     src_basename(filename), line, "%u created %u (%u/%u) for %u", fixture, hid, index, handle->generation, ptr
    // );
    return hid;
}

void _destroy_handle(struct Fixture* fixture, HandleID hid, const char* filename, int line) {
    size_t index = (size_t)(hid >> HID_BITS);
    HID_HALF generation = (HID_HALF)(hid & HID_LIMIT);

    // Sanity check
    if (index >= fixture->capacity) {
        log_generic(
            src_basename(filename), line, "!! %u destroying invalid handle %u (%u/%u)?", fixture, hid, index, generation
        );
        return;
    }

    struct Handle* handle = &fixture->handles[index];
    if (handle->ptr == NULL || handle->generation != generation) {
        log_generic(
            src_basename(filename), line, "!! %u destroying invalid handle %u (%u/%u)?", fixture, hid, index, generation
        );
        return;
    }

    handle->ptr = NULL;
    if (index < fixture->next)
        fixture->next = index;
    --fixture->size;

    // log_generic(src_basename(filename), line, "%u destroyed %u (%u/%u)", fixture, hid, index, generation);
}

struct Handle* hid_to_handle(struct Fixture* fixture, HandleID hid) {
    size_t index = (size_t)(hid >> HID_BITS);
    HID_HALF generation = (HID_HALF)(hid & HID_LIMIT);

    // Sanity check
    if (index >= fixture->capacity)
        return NULL;

    struct Handle* handle = &fixture->handles[index];
    return (handle->ptr == NULL || handle->generation != generation) ? NULL : handle;
}

void* hid_to_pointer(struct Fixture* fixture, HandleID hid) {
    size_t index = (size_t)(hid >> HID_BITS);
    HID_HALF generation = (HID_HALF)(hid & HID_LIMIT);

    // Sanity check
    if (index >= fixture->capacity)
        return NULL;

    struct Handle* handle = &fixture->handles[index];
    return (handle->generation != generation) ? NULL : handle->ptr; // God-knows-what
}

// Abstract hash maps
// You can either free values manually or nuke them alongside the maps.
// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
#define HASH_CAPACITY 2
#define FNV_OFFSET 0x811c9dc5
#define FNV_PRIME 0x01000193

static uint32_t hash_key(const char* key) {
    uint32_t hash = FNV_OFFSET;
    for (const char* p = key; *p; p++) {
        hash ^= (uint32_t)(unsigned char)(*p);
        hash *= FNV_PRIME;
    }
    return hash;
}

struct HashMap* _create_hash_map(const char* filename, int line) {
    struct HashMap* map = _lame_alloc(sizeof(struct HashMap), filename, line);
    map->count = map->length = 0;
    map->capacity = HASH_CAPACITY;

    const size_t size = HASH_CAPACITY * sizeof(struct KeyValuePair);
    map->items = _lame_alloc(size, filename, line);
    _lame_set(map->items, 0, size, filename, line);

    return map;
}

void _destroy_hash_map(struct HashMap* map, bool nuke, const char* filename, int line) {
    for (size_t i = 0; i < map->capacity; i++) {
        struct KeyValuePair* kvp = &map->items[i];
        if (kvp->key != NULL && kvp->key != HASH_TOMBSTONE)
            _lame_free((void**)&kvp->key, filename, line);
        if (nuke && kvp->value != NULL)
            _lame_free(&kvp->value, filename, line);
    }

    _lame_free((void**)&map->items, filename, line);
    _lame_free((void**)&map, filename, line);
}

static bool to_hash_map_direct(
    struct KeyValuePair* items, size_t* count, size_t* length, size_t capacity, const char* key, void* value, bool nuke,
    const char* filename, int line
) {
    size_t index = (size_t)hash_key(key) % capacity;
    struct KeyValuePair* kvp = &(items[index]);
    while (kvp->key != NULL) {
        if (kvp->key != HASH_TOMBSTONE && SDL_strcmp(key, kvp->key) == 0) {
            if (kvp->value != NULL) {
                if (nuke)
                    _lame_free(&kvp->value, filename, line);
                else
                    return false;
            }
            kvp->value = value;
            return true;
        }

        index = (index + 1) % capacity;
        kvp = &(items[index]);
    }

    items[index].key = count == NULL ? (char*)key : SDL_strdup(key);
    items[index].value = value;
    if (count != NULL) {
        (*count)++;
        (*length)++;
    }
    return true;
}

static void expand_hash_map(struct HashMap* map, const char* filename, int line) {
    const size_t new_capacity = map->capacity * 2;
    if (new_capacity < map->capacity)
        log_fatal(src_basename(filename), line, "Capacity overflow in HashMap");

    const size_t old_capacity = map->capacity;
    const size_t new_size = new_capacity * sizeof(struct KeyValuePair);
    struct KeyValuePair* items = _lame_alloc(new_size, filename, line);
    _lame_set(items, 0, new_size, filename, line);

    for (size_t i = 0; i < map->capacity; i++) {
        struct KeyValuePair* kvp = &(map->items[i]);
        if (kvp->key != NULL) {
            if (kvp->key == HASH_TOMBSTONE)
                map->length--;
            else
                to_hash_map_direct(items, NULL, NULL, new_capacity, kvp->key, kvp->value, false, filename, line);
        }
    }

    _lame_free((void**)&(map->items), filename, line);
    map->items = items;
    map->capacity = new_capacity;
    log_generic(src_basename(filename), line, "Increased HashMap capacity to %u", new_capacity);
}

bool _to_hash_map(struct HashMap* map, const char* key, void* value, bool nuke, const char* filename, int line) {
    if (map->length >= map->capacity / 2)
        expand_hash_map(map, filename, line);

    return to_hash_map_direct(
        map->items, &(map->count), &(map->length), map->capacity, key, value, nuke, filename, line
    );
}

void* from_hash_map(struct HashMap* map, const char* key) {
    size_t index = (size_t)hash_key(key) % map->capacity;
    const struct KeyValuePair* kvp = &map->items[index];
    while (kvp->key != NULL) {
        if (kvp->key != HASH_TOMBSTONE && SDL_strcmp(key, kvp->key) == 0)
            return kvp->value;

        index = (index + 1) % map->capacity;
        kvp = &map->items[index];
    }

    return NULL;
}

void* _pop_hash_map(struct HashMap* map, const char* key, bool nuke, const char* filename, int line) {
    size_t index = (size_t)hash_key(key) % map->capacity;
    struct KeyValuePair* kvp = &map->items[index];
    while (kvp->key != NULL) {
        if (kvp->key != HASH_TOMBSTONE && SDL_strcmp(key, kvp->key) == 0) {
            _lame_free((void**)(&(kvp->key)), filename, line);
            kvp->key = HASH_TOMBSTONE;
            void* value = kvp->value;
            kvp->value = NULL;
            if (nuke && value != NULL)
                _lame_free(&value, filename, line);
            map->count--;
            return value;
        }

        index = (index + 1) % map->capacity;
        kvp = &map->items[index];
    }

    return NULL;
}

static uint32_t int_hash_key(uint32_t key) {
    uint32_t hash = FNV_OFFSET;
    for (size_t i = 0; i < sizeof(uint32_t); i++) {
        hash ^= (uint32_t)(key >> (i << 3)) & 0xFF;
        hash *= FNV_PRIME;
    }
    return hash;
}

struct IntMap* _create_int_map(const char* filename, int line) {
    struct IntMap* map = _lame_alloc(sizeof(struct IntMap), filename, line);
    map->count = map->length = 0;
    map->capacity = HASH_CAPACITY;

    const size_t size = HASH_CAPACITY * sizeof(struct IKeyValuePair);
    map->items = _lame_alloc(size, filename, line);
    _lame_set(map->items, 0, size, filename, line);

    return map;
}

void _destroy_int_map(struct IntMap* map, bool nuke, const char* filename, int line) {
    for (size_t i = 0; i < map->capacity; i++) {
        struct IKeyValuePair* kvp = &map->items[i];
        if (nuke && kvp->value != NULL)
            _lame_free(&kvp->value, filename, line);
    }

    _lame_free((void**)&map->items, filename, line);
    _lame_free((void**)&map, filename, line);
}

static bool to_int_map_direct(
    struct IKeyValuePair* items, size_t* count, size_t* length, size_t capacity, uint32_t key, void* value, bool nuke,
    const char* filename, int line
) {
    size_t index = (size_t)int_hash_key(key) % capacity;
    struct IKeyValuePair* kvp = &(items[index]);
    while (kvp->state != IKVP_NONE) {
        if (kvp->state != IKVP_DELETED && key == kvp->key) {
            if (kvp->value != NULL) {
                if (nuke)
                    _lame_free(&kvp->value, filename, line);
                else
                    return false;
            }
            kvp->value = value;
            return true;
        }

        index = (index + 1) % capacity;
        kvp = &(items[index]);
    }

    items[index].state = IKVP_OCCUPIED;
    items[index].key = key;
    items[index].value = value;
    if (count != NULL) {
        (*count)++;
        (*length)++;
    }
    return true;
}

static void expand_int_map(struct IntMap* map, const char* filename, int line) {
    const size_t new_capacity = map->capacity * 2;
    if (new_capacity < map->capacity)
        log_fatal(src_basename(filename), line, "Capacity overflow in IntMap");

    const size_t old_capacity = map->capacity;
    const size_t new_size = new_capacity * sizeof(struct IKeyValuePair);
    struct IKeyValuePair* items = _lame_alloc(new_size, filename, line);
    _lame_set(items, 0, new_size, filename, line);

    for (size_t i = 0; i < map->capacity; i++) {
        struct IKeyValuePair* kvp = &(map->items[i]);
        if (kvp->state != IKVP_NONE) {
            if (kvp->state == IKVP_DELETED)
                map->length--;
            else
                to_int_map_direct(items, NULL, NULL, new_capacity, kvp->key, kvp->value, false, filename, line);
        }
    }

    _lame_free((void**)&(map->items), filename, line);
    map->items = items;
    map->capacity = new_capacity;
    log_generic(src_basename(filename), line, "Increased IntMap capacity to %u", new_capacity);
}

bool _to_int_map(struct IntMap* map, uint32_t key, void* value, bool nuke, const char* filename, int line) {
    if (map->length >= map->capacity / 2)
        expand_int_map(map, filename, line);

    return to_int_map_direct(
        map->items, &(map->count), &(map->length), map->capacity, key, value, nuke, filename, line
    );
}

void* from_int_map(struct IntMap* map, uint32_t key) {
    size_t index = (size_t)int_hash_key(key) % map->capacity;
    const struct IKeyValuePair* kvp = &map->items[index];
    while (kvp->state != IKVP_NONE) {
        if (kvp->state != IKVP_DELETED && key == kvp->key)
            return kvp->value;

        index = (index + 1) % map->capacity;
        kvp = &map->items[index];
    }

    return NULL;
}

void* _pop_int_map(struct IntMap* map, uint32_t key, bool nuke, const char* filename, int line) {
    size_t index = (size_t)int_hash_key(key) % map->capacity;
    struct IKeyValuePair* kvp = &map->items[index];
    while (kvp->state != IKVP_NONE) {
        if (kvp->state != IKVP_DELETED && key == kvp->key) {
            kvp->state = IKVP_DELETED;
            void* value = kvp->value;
            kvp->value = NULL;
            if (nuke && value != NULL)
                _lame_free(&value, filename, line);
            map->count--;
            return value;
        }

        index = (index + 1) % map->capacity;
        kvp = &map->items[index];
    }

    return NULL;
}
