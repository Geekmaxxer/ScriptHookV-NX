#pragma once
#include "plugin_files.h"

#define SHV_NX_CATALOG_ABI_VERSION 1u
#define SHV_NX_CATALOG_PAGE_ENTRIES 16u
#define SHV_NX_CATALOG_MAX_ENTRIES 4096u
#define SHV_NX_CATALOG_NAME_BYTES 769u
#define SHV_NX_CATALOG_COMPATIBLE_NAME 1u
#define SHV_NX_CATALOG_DIRECTORY 1u
#define SHV_NX_CATALOG_FILE 2u

#ifdef __cplusplus
extern "C" {
#endif

// Names are copied in full. COMPATIBLE_NAME describes one filename component;
// the assembled relative path must still satisfy the file service path limits.
typedef struct ShvNxDirectoryEntry {
    uint32_t kind, flags;
    uint64_t bytes;
    char name[SHV_NX_CATALOG_NAME_BYTES];
    uint8_t reserved[7];
} ShvNxDirectoryEntry;

typedef struct ShvNxDirectoryPage {
    uint32_t count, next_offset;
} ShvNxDirectoryPage;

// Independent optional table. Each page reopens its directory and skips the
// requested number of physical entries; no handle or cursor is retained.
typedef struct ShvNxCatalog {
    uint32_t size, abi;
    ShvNxFileStatus (*list)(void*, const char*, uint32_t, ShvNxDirectoryEntry*,
        uint32_t, ShvNxDirectoryPage*);
} ShvNxCatalog;

SHV_NX_API bool shvPluginBindCatalog(const ShvNxCatalog* catalog);
SHV_NX_API bool scriptCatalogAvailable(void);
// path="" lists the plugin's data root. next_offset=0 indicates EOF. Discard
// output entries on failure; a bound call clears the validated page result.
SHV_NX_API ShvNxFileStatus scriptDirectoryList(const char* path, uint32_t offset,
    ShvNxDirectoryEntry* entries, uint32_t capacity, ShvNxDirectoryPage* page);

#ifdef __cplusplus
}
#endif
