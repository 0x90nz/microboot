/**
 * Fairly simple hash-table implementation.
 *
 * Uses the FNV-1a hash to get an index into the table, and then performs a
 * linear probe until the matching entry is found. Will dynamically resize, so
 * will not run out of space so long as there is memory available.
 */

#include "htbl.h"
#include <stddef.h>
#include "alloc.h"
#include "stdlib.h"

// the number of entries the hash table starts with, note though that this may
// further be expanded if the hash table gets full enough.
//
// NOTE: this MUST be a power of two, as to find the index an entry is stored
// at, we use `hash & (capacity - 1)` using capacity to create a mask. Capacity
// is maintained as a power of two by the expansion, which is always by *2, but
// this initial value must be set with care.
#define HTBL_INITIAL_CAPACITY       32

struct htbl_entry {
    const char* key;
    void* value;
};

struct htbl {
    // array of entries, `capacity` long
    struct htbl_entry* entries;
    // the number of elements in the `entries` array
    size_t capacity;
    // the total number of items stored in the table currently
    size_t length;
};

htbl_t* htbl_create()
{
    htbl_t* table = kalloc(sizeof(*table));

    table->capacity = HTBL_INITIAL_CAPACITY;
    table->length = 0;
    table->entries = kallocz(table->capacity * sizeof(struct htbl_entry));

    return table;
}

void htbl_destroy(htbl_t* table)
{
    ASSERT(table && table->entries, "NULL table or entries");

    // free all the keys (which we duplicated, so they're our problem). values
    // are left to the user
    for (size_t i = 0; i < table->capacity; i++) {
        if (table->entries[i].key != NULL) {
            // cast to void* so we can free in spite of the const qualifier
            kfree((void*)table->entries[i].key);
        }
    }

    kfree(table->entries);
    kfree(table);
}

// FNV-1a hash algorithm, 32 bit; seems to have a reasonable distribution
// see: http://www.isthe.com/chongo/tech/comp/fnv/index.html
#define FNV_OFFSET_32   2166136261
#define FNV_PRIME_32    16777619

static uint32_t fnv1a32_hash(const char* key)
{
    uint32_t hash = FNV_OFFSET_32;
    const uint8_t* s = (const uint8_t*)key;

    while (*s) {
        hash ^= *s++;
        hash *= FNV_PRIME_32;
    }

    return hash;
}

void* htbl_get(htbl_t* table, const char* key)
{
    ASSERT(table, "NULL table");
    ASSERT(key, "NULL key");

    uint32_t hash = fnv1a32_hash(key);
    // bound the index within the table of key at the size of the table
    size_t index = (size_t)(hash & (uint32_t)(table->capacity - 1));

    // go through the table, starting at the provided index, looking for the
    // key. if we reach the end, wrap around.
    //
    // this requires that the table be properly formed, i.e. that not all
    // entires are full
    while (table->entries[index].key != NULL) {
        if (strcmp(key, table->entries[index].key) == 0)
            return table->entries[index].value;

        index++;
        if (index >= table->capacity) index = 0;
    }

    // we looked at all the values and found nothing, so this mapping must not
    // exist within the table
    return NULL;
}

// at some point, the table will be full of entries (or realistically, at some
// threshold before that so we reduce the amount of linear probing we need to
// do) and we need to expand it.
//
// this allocates a new `entries` member and copies all the existing entries to
// it, before freeing the old entries.
//
// returns non-zero on success, zero otherwise.
static int htbl_expand(htbl_t* table)
{
    size_t extended_capacity = table->capacity * 2;
    struct htbl_entry* extended_entries = kallocz(extended_capacity * sizeof(*extended_entries));

    // create a temporary table so we can use `htbl_put` to fill the new entry
    // table
    htbl_t temp_tbl = {
        .entries = extended_entries,
        .capacity = extended_capacity,
        .length = 0,
    };
    for (size_t i = 0; i < table->capacity; i++) {
        struct htbl_entry entry = table->entries[i];
        if (entry.key != NULL)
            htbl_put(&temp_tbl, entry.key, entry.value);
    }

    kfree(table->entries);
    table->entries = extended_entries;
    table->capacity = extended_capacity;
    return 1;
}

int htbl_put(htbl_t* table, const char* key, void* value)
{
    ASSERT(table, "NULL table");
    ASSERT(key, "NULL key");

    // if the table is getting crowded enough, expand it first
    if (table->length >= table->capacity / 2)
        htbl_expand(table);

    // actually set the entry
    uint32_t hash = fnv1a32_hash(key);
    // bound the index within the table of key at the size of the table
    size_t index = (size_t)(hash & (uint32_t)(table->capacity - 1));

    // see if we can find the key within the table already, do this similar to
    // htbl_get
    while (table->entries[index].key != NULL) {
        if (strcmp(key, table->entries[index].key) == 0) {
            table->entries[index].value = value;
            return 1;
        }

        index++;
        if (index >= table->capacity) index = 0;
    }

    // unable to find the key within the table, copy and insert
    table->entries[index].key = strdup(key);
    table->entries[index].value = value;
    table->length++;
    return 1;
}


