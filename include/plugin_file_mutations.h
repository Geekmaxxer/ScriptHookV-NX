#pragma once
#include "plugin_files.h"

#define SHV_NX_FILE_MUTATIONS_ABI_VERSION 1u

#ifdef __cplusplus
extern "C" {
#endif

// Optional independent table. Operations affect files inside one owner data
// namespace. Rename never replaces an existing destination; no recursion.
typedef struct ShvNxFileMutations {
    uint32_t size, abi;
    ShvNxFileStatus (*rename_file)(void*, const char*, const char*);
    ShvNxFileStatus (*delete_file)(void*, const char*);
    // Optional sized tail. Older ABI-1 tables end before this member.
    ShvNxFileStatus (*resize_file)(void*, const char*, uint64_t);
} ShvNxFileMutations;

SHV_NX_API bool shvPluginBindFileMutations(const ShvNxFileMutations* mutations);
SHV_NX_API bool scriptFileMutationsAvailable(void);
// Even an error may follow a completed mutation. Reinspect paths before retry.
SHV_NX_API ShvNxFileStatus scriptFileRename(const char* from, const char* to);
SHV_NX_API ShvNxFileStatus scriptFileDelete(const char* path);
SHV_NX_API bool scriptFileResizeAvailable(void);
// Explicitly destructive: shrinking discards bytes. Extension bytes have no
// specified contents. No replacement, directory creation or power-loss guarantee.
SHV_NX_API ShvNxFileStatus scriptFileResize(const char* path, uint64_t bytes);

#ifdef __cplusplus
}
#endif
