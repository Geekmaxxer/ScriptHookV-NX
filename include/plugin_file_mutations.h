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
} ShvNxFileMutations;

SHV_NX_API bool shvPluginBindFileMutations(const ShvNxFileMutations* mutations);
SHV_NX_API bool scriptFileMutationsAvailable(void);
// Even an error may follow a completed mutation. Reinspect paths before retry.
SHV_NX_API ShvNxFileStatus scriptFileRename(const char* from, const char* to);
SHV_NX_API ShvNxFileStatus scriptFileDelete(const char* path);

#ifdef __cplusplus
}
#endif
