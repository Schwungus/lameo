#include <SDL3/SDL_stdinc.h>

#include "log.h"
#include "mem.h"

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

void* _lame_copy(void* dest, const void* src, size_t n, const char* filename, int line) {
    if (dest == NULL || src == NULL)
        log_fatal(src_basename(filename), line, "Copying from %u to %u?", src, dest);
    return SDL_memcpy(dest, src, n);
}

void _lame_realloc(void** ptr, size_t size, const char* filename, int line) {
    if (ptr == NULL || *ptr == NULL)
        log_fatal(src_basename(filename), line, "Resizing a null pointer?");
    if (!size)
        log_fatal(src_basename(filename), line, "Reallocating to 0 bytes?");

    *ptr = SDL_realloc(*ptr, size);
    if (*ptr == NULL)
        log_fatal(src_basename(filename), line, "Reallocation failed");
}

struct Fixture* create_fixture() {
    struct Fixture* fixture = lame_alloc(sizeof(struct Fixture));
    fixture->size = 0;
    fixture->capacity = 1;
    fixture->next = 0;

    // Handles array
    fixture->handles = lame_alloc(fixture->capacity * sizeof(struct Handle));
    fixture->handles[0].ptr = NULL;
    fixture->handles[0].generation = 0;

    return fixture;
}

void destroy_fixture(struct Fixture* fixture) {
    lame_free(&fixture);
}

HandleID create_handle(struct Fixture* fixture, void* ptr) {
    if (ptr == NULL)
        FATAL("Creating a handle for a null pointer?");
    if (fixture->size >= HID_LIMIT)
        FATAL("Out of handles (%u >= %u)", fixture->size, HID_LIMIT);

    // Expand handles array on demand
    size_t old_capacity = fixture->capacity;
    if (fixture->next >= old_capacity) {
        if (old_capacity >= HID_LIMIT)
            FATAL("Out of handle capacity (%u >= %u)", old_capacity, HID_LIMIT);

        size_t new_capacity = fixture->capacity * 2;
        lame_realloc(&fixture->handles, new_capacity * sizeof(struct Handle));
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
        WARN("%u handle index %u generation wrapped, expect undefined behavior from stale handle IDs", fixture, index);
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
    INFO("%u created %u (%u/%u) for %u", fixture, hid, index, handle->generation, ptr);
    return hid;
}

void destroy_handle(struct Fixture* fixture, HandleID hid) {
    size_t index = (size_t)(hid >> HID_BITS);
    HID_HALF generation = (HID_HALF)(hid & HID_LIMIT);

    struct Handle* handle = &fixture->handles[index];
    if (handle->ptr == NULL || handle->generation != generation)
        FATAL("Destroying an invalid handle at index %u, generation %u", index, generation);

    handle->ptr = NULL;
    if (index < fixture->next)
        fixture->next = index;
    --fixture->size;

    INFO("%u destroyed %u (%u/%u)", fixture, hid, index, generation);
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
