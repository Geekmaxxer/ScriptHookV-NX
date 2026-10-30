#pragma once
#include "main.h"

#define SHV_NX_FILES_ABI_VERSION 1u
#define SHV_NX_FILE_MAX_BYTES (64u * 1024u * 1024u)
#define SHV_NX_FILE_MAX_CHUNK 65536u
#define SHV_NX_FILE_MAX_PATH 79u

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ShvNxFileStatus {
    ShvNxFile_Ok = 0,
    ShvNxFile_Unavailable,
    ShvNxFile_InvalidArgument,
    ShvNxFile_WrongOwner,
    ShvNxFile_NotFound,
    ShvNxFile_AlreadyExists,
    ShvNxFile_OutOfRange,
    ShvNxFile_IoError,
    ShvNxFile_IncompleteRead,
    ShvNxFile_DirectoryLimit,
    ShvNxFile_InvalidEntry,
    ShvNxFile_VerificationFailed
} ShvNxFileStatus;

// Optional, independent ABI. No handles escape an operation. Paths are relative
// to settings/<plugin-id>/; parent directories must be created explicitly.
typedef struct ShvNxFiles {
    uint32_t size, abi;
    ShvNxFileStatus (*size_of)(void*, const char*, uint64_t*);
    ShvNxFileStatus (*read_at)(void*, const char*, uint64_t, void*, size_t, size_t*);
    ShvNxFileStatus (*write_at)(void*, const char*, uint64_t, const void*, size_t);
    ShvNxFileStatus (*create)(void*, const char*, uint64_t);
    ShvNxFileStatus (*directory)(void*, const char*);
} ShvNxFiles;

SHV_NX_API bool shvPluginBindFiles(const ShvNxFiles* files);
SHV_NX_API bool scriptFilesAvailable(void);
SHV_NX_API ShvNxFileStatus scriptFileSize(const char* path, uint64_t* bytes);
SHV_NX_API ShvNxFileStatus scriptFileReadAt(const char* path, uint64_t offset,
    void* output, size_t capacity, size_t* bytes);
SHV_NX_API ShvNxFileStatus scriptFileWriteAt(const char* path, uint64_t offset,
    const void* data, size_t bytes);
// Exclusive creation: an existing file is never truncated or replaced.
SHV_NX_API ShvNxFileStatus scriptFileCreate(const char* path, uint64_t bytes);
SHV_NX_API ShvNxFileStatus scriptDirectoryCreate(const char* path);

#ifdef __cplusplus
}
#endif
