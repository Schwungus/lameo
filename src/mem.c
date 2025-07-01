#include "mem.h"
#include "log.h"

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
// These are destructive (frees pointers themselves).
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
    map->count = 0;
    map->capacity = HASH_CAPACITY;

    const size_t size = HASH_CAPACITY * sizeof(struct KeyValuePair);
    map->items = _lame_alloc(size, filename, line);
    _lame_set(map->items, 0, size, filename, line);

    return map;
}

void _destroy_hash_map(struct HashMap* map, const char* filename, int line) {
    for (size_t i = 0; i < map->capacity; i++) {
        struct KeyValuePair* kvp = &map->items[i];
        if (kvp->key != NULL)
            _lame_free((void**)&kvp->key, filename, line);
        if (kvp->value != NULL)
            _lame_free(&kvp->value, filename, line);
    }

    _lame_free((void**)&map->items, filename, line);
    _lame_free((void**)&map, filename, line);
}

void _to_hash_map(struct HashMap* map, const char* key, void* value, const char* filename, int line) {
    if (map->count >= map->capacity / 2) {
        size_t new_capacity = map->capacity * 2;
        if (new_capacity < map->capacity)
            log_fatal(filename, line, "Capacity overflow in hash table");

        size_t old_capacity = map->capacity;
        _lame_realloc((void**)&map->items, new_capacity, filename, line);
        _lame_set(
            map->items + old_capacity, 0, (new_capacity - old_capacity) * sizeof(struct KeyValuePair), filename, line
        );
        map->capacity = new_capacity;
    }

    size_t index = (size_t)hash_key(key) % map->capacity;
    struct KeyValuePair* kvp = &map->items[index];
    while (kvp->key != NULL) {
        if (SDL_strcmp(key, kvp->key) == 0) {
            if (kvp->value != NULL)
                _lame_free(&kvp->value, filename, line);
            kvp->value = value;
            return;
        }

        index = (index + 1) % map->capacity;
        kvp = &map->items[index];
    }

    map->items[index].key = SDL_strdup(key);
    map->items[index].value = value;
}

void* _from_hash_map(struct HashMap* map, const char* key, const char* filename, int line) {
    size_t index = (size_t)hash_key(key) % map->capacity;
    struct KeyValuePair* kvp = &map->items[index];
    while (kvp->key != NULL) {
        if (SDL_strcmp(key, kvp->key) == 0)
            return kvp->value;

        index = (index + 1) % map->capacity;
        kvp = &map->items[index];
    }

    return NULL;
}
