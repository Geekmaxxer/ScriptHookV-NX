// Read-only C streams over the versioned, namespaced file/resource services.
#include "shv/stdio.h"
#include "plugin_files.h"
#include "plugin_memory.h"
#include "plugin_resources.h"
#include <errno.h>
#include <limits.h>
#include <new>

struct ShvNxReadStream {
    ShvNxReadStream* next;
    ShvNxReadStream* previous;
    ShvNxResourceHandle resource;
    uint64_t extent, position, cache_start;
    size_t cached;
    bool eof, error;
    char path[SHV_NX_FILE_MAX_PATH + 1];
    unsigned char buffer[512];
};

namespace {
using Stream = ShvNxReadStream;
Stream* first;
static_assert(sizeof(long) == 8, "Streams require AArch64 long offsets");

int StatusError(ShvNxFileStatus status) {
    if (status == ShvNxFile_NotFound) return ENOENT;
    if (status == ShvNxFile_InvalidArgument) return EINVAL;
    if (status == ShvNxFile_OutOfRange) return EOVERFLOW;
    if (status == ShvNxFile_Unavailable) return ENOSYS;
    if (status == ShvNxFile_WrongOwner) return EPERM;
    return EIO;
}

Stream* Find(FILE* pointer) {
    // Validate ownership/thread before traversing NRO-local registry storage.
    ShvNxMemoryStats stats{};
    if (!scriptGetMemoryStats(&stats)) { errno = EPERM; return nullptr; }
    for (auto* stream = first; stream; stream = stream->next) {
        if (reinterpret_cast<FILE*>(stream) == pointer && scriptResourceActive(stream->resource)) return stream;
    }
    errno = EBADF;
    return nullptr;
}

void Error(Stream& stream, int value) { stream.error = true; errno = value; }

void Close(void* context) {
    auto* stream = static_cast<Stream*>(context);
    if (stream->previous) stream->previous->next = stream->next;
    else first = stream->next;
    if (stream->next) stream->next->previous = stream->previous;
    stream->~ShvNxReadStream();
    (void)scriptFree(stream);
}

bool Extent(Stream& stream) {
    uint64_t bytes = 0;
    const auto status = scriptFileSize(stream.path, &bytes);
    if (status != ShvNxFile_Ok) { Error(stream, StatusError(status)); return false; }
    if (bytes != stream.extent) { Error(stream, EIO); return false; }
    return true;
}

bool Fill(Stream& stream) {
    stream.cached = 0;
    if (!Extent(stream)) return false;
    if (stream.position >= stream.extent) { stream.eof = true; return false; }
    const auto remaining = stream.extent - stream.position;
    const size_t wanted = remaining < sizeof(stream.buffer) ? size_t(remaining) : sizeof(stream.buffer);
    size_t got = 0;
    const auto status = scriptFileReadAt(stream.path, stream.position, stream.buffer, wanted, &got);
    // Refuse partial fills: output and logical position remain unchanged for
    // this fill. Bytes delivered by earlier successful fills stay consumed.
    if (status != ShvNxFile_Ok || got != wanted) {
        Error(stream, status == ShvNxFile_Ok ? EIO : StatusError(status));
        return false;
    }
    stream.cache_start = stream.position;
    stream.cached = got;
    return true;
}

size_t Read(Stream& stream, unsigned char* out, size_t bytes) {
    size_t delivered = 0;
    if (stream.eof) return 0;
    while (delivered < bytes) {
        if (!stream.cached || stream.position < stream.cache_start ||
            stream.position - stream.cache_start >= stream.cached) {
            if (!Fill(stream)) break;
        }
        const size_t offset = size_t(stream.position - stream.cache_start);
        size_t count = stream.cached - offset;
        if (count > bytes - delivered) count = bytes - delivered;
        for (size_t i = 0; i < count; ++i) out[delivered + i] = stream.buffer[offset + i];
        stream.position += count;
        delivered += count;
    }
    return delivered;
}
}

extern "C" {
FILE* fopen(const char* path, const char* mode) {
    if (!path || !mode || mode[0] != 'r' || (mode[1] && (mode[1] != 'b' || mode[2]))) {
        errno = EINVAL; return nullptr;
    }
    if (!scriptFilesAvailable() || !scriptResourcesAvailable() || !scriptMemoryAvailable()) {
        errno = ENOSYS; return nullptr;
    }
    size_t length = 0;
    while (length <= SHV_NX_FILE_MAX_PATH && path[length]) ++length;
    if (!length || length > SHV_NX_FILE_MAX_PATH) { errno = EINVAL; return nullptr; }
    uint64_t extent = 0;
    const auto status = scriptFileSize(path, &extent);
    if (status != ShvNxFile_Ok) { errno = StatusError(status); return nullptr; }
    void* storage = scriptAllocate(sizeof(Stream), alignof(Stream), 0);
    if (!storage) { errno = ENOMEM; return nullptr; }
    auto* stream = ::new(storage) Stream{};
    for (size_t i = 0; i <= length; ++i) stream->path[i] = path[i];
    stream->extent = extent;
    stream->resource = scriptResourceRegister(Close, stream);
    if (!stream->resource) { stream->~ShvNxReadStream(); (void)scriptFree(stream); errno = ENOMEM; return nullptr; }
    stream->next = first;
    if (first) first->previous = stream;
    first = stream;
    return reinterpret_cast<FILE*>(stream);
}

int fclose(FILE* pointer) {
    auto* stream = Find(pointer);
    if (!stream) return EOF;
    if (scriptResourceRelease(stream->resource)) return 0;
    errno = EBUSY;
    return EOF;
}

size_t fread(void* output, size_t width, size_t count, FILE* pointer) {
    auto* stream = Find(pointer);
    if (!stream || !width || !count) return 0;
    if (!output) { Error(*stream, EINVAL); return 0; }
    if (count > SIZE_MAX / width) { Error(*stream, EOVERFLOW); return 0; }
    return Read(*stream, static_cast<unsigned char*>(output), width * count) / width;
}

int fgetc(FILE* pointer) {
    auto* stream = Find(pointer);
    unsigned char byte = 0;
    return stream && Read(*stream, &byte, 1) == 1 ? int(byte) : EOF;
}
int getc(FILE* stream) { return fgetc(stream); }

char* fgets(char* output, int capacity, FILE* pointer) {
    auto* stream = Find(pointer);
    if (!stream) return nullptr;
    if (!output || capacity <= 0) { Error(*stream, EINVAL); return nullptr; }
    if (capacity == 1) { output[0] = 0; return output; }
    size_t got = 0;
    bool read_error = false;
    while (got < size_t(capacity - 1)) {
        unsigned char byte = 0;
        if (!Read(*stream, &byte, 1)) { read_error = !stream->eof; break; }
        output[got++] = char(byte);
        if (byte == '\n') break;
    }
    if (!got || read_error) return nullptr;
    output[got] = 0;
    return output;
}

int fseek(FILE* pointer, long offset, int origin) {
    auto* stream = Find(pointer);
    if (!stream) return -1;
    uint64_t base = 0;
    if (origin == SEEK_CUR) base = stream->position;
    else if (origin == SEEK_END) {
        if (!Extent(*stream)) return -1;
        base = stream->extent;
    } else if (origin != SEEK_SET) { Error(*stream, EINVAL); return -1; }
    uint64_t position;
    if (offset < 0) {
        const uint64_t distance = uint64_t(-(offset + 1)) + 1;
        if (distance > base) { Error(*stream, EINVAL); return -1; }
        position = base - distance;
    } else {
        if (uint64_t(offset) > SHV_NX_FILE_MAX_BYTES - base) { Error(*stream, EOVERFLOW); return -1; }
        position = base + uint64_t(offset);
    }
    stream->position = position;
    stream->cached = 0;
    stream->eof = false;
    return 0;
}

long ftell(FILE* pointer) { auto* stream = Find(pointer); return stream ? long(stream->position) : -1; }
int feof(FILE* pointer) { auto* stream = Find(pointer); return stream && stream->eof; }
int ferror(FILE* pointer) { auto* stream = Find(pointer); return !stream || stream->error; }
void clearerr(FILE* pointer) { if (auto* stream = Find(pointer)) stream->eof = stream->error = false; }
void rewind(FILE* pointer) { if (fseek(pointer, 0, SEEK_SET) == 0) clearerr(pointer); }
}
