// C streams over the versioned, namespaced file/resource services.
#include "shv/stdio.h"
#include "plugin_files.h"
#include "plugin_memory.h"
#include "plugin_resources.h"
#include "plugin_file_mutations.h"
#include <errno.h>
#include <limits.h>
#include <new>
#include <stdarg.h>

struct ShvNxReadStream {
    ShvNxReadStream* next;
    ShvNxReadStream* previous;
    ShvNxResourceHandle resource;
    uint64_t extent, position, cache_start;
    size_t cached;
    bool eof, error, readable, writable, append;
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
    if (status == ShvNxFile_AlreadyExists) return EEXIST;
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
    if (!stream.readable) { Error(stream, EBADF); return 0; }
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
size_t Write(Stream& stream, const unsigned char* input, size_t bytes) {
    if (!stream.writable) { Error(stream, EBADF); return 0; }
    uint64_t size = 0;
    auto status = scriptFileSize(stream.path, &size);
    if (status != ShvNxFile_Ok) { Error(stream, StatusError(status)); return 0; }
    if (size > SHV_NX_FILE_MAX_BYTES) { Error(stream, EOVERFLOW); return 0; }
    if (stream.append) stream.position = size;
    else if (size != stream.extent) { Error(stream, EIO); return 0; }
    stream.extent = size;
    if (stream.position > SHV_NX_FILE_MAX_BYTES || bytes > SHV_NX_FILE_MAX_BYTES - stream.position) {
        Error(stream, EOVERFLOW); return 0;
    }
    stream.cached = 0;
    // Explicitly zero holes; the resize service does not promise initialized
    // extension bytes. A failed mutation can leave a partially changed file.
    if (stream.position > stream.extent) {
        const auto old = stream.extent;
        status = scriptFileResize(stream.path, stream.position);
        if (status != ShvNxFile_Ok) { Error(stream, StatusError(status)); return 0; }
        stream.extent = stream.position;
        static const unsigned char zeros[4096]{};
        for (uint64_t at = old; at < stream.position;) {
            const size_t count = stream.position - at < sizeof(zeros) ? size_t(stream.position - at) : sizeof(zeros);
            status = scriptFileWriteAt(stream.path, at, zeros, count);
            if (status != ShvNxFile_Ok) { Error(stream, StatusError(status)); return 0; }
            at += count;
        }
    }
    size_t delivered = 0;
    while (delivered < bytes) {
        const size_t count = bytes - delivered < SHV_NX_FILE_MAX_CHUNK ? bytes - delivered : SHV_NX_FILE_MAX_CHUNK;
        const auto end = stream.position + count;
        if (end > stream.extent) {
            status = scriptFileResize(stream.path, end);
            if (status != ShvNxFile_Ok) { Error(stream, StatusError(status)); break; }
            stream.extent = end;
        }
        status = scriptFileWriteAt(stream.path, stream.position, input + delivered, count);
        if (status != ShvNxFile_Ok) { Error(stream, StatusError(status)); break; }
        stream.position = end;
        delivered += count;
    }
    return delivered;
}
struct Mode { char initial; bool read, write, append, exclusive; };
bool ParseMode(const char* text, Mode& mode) {
    if (!text || (text[0] != 'r' && text[0] != 'w' && text[0] != 'a')) return false;
    mode.initial = text[0];
    bool binary = false, update = false;
    for (size_t i = 1; text[i]; ++i) {
        if (i > 3) return false;
        if (text[i] == 'b' && !binary) binary = true;
        else if (text[i] == '+' && !update) update = true;
        else if (text[i] == 'x' && !mode.exclusive && mode.initial == 'w') mode.exclusive = true;
        else return false;
    }
    mode.read = mode.initial == 'r' || update;
    mode.write = mode.initial != 'r' || update;
    mode.append = mode.initial == 'a';
    return true;
}
}

extern "C" {
FILE* fopen(const char* path, const char* mode) {
    Mode parsed{};
    if (!path || !ParseMode(mode, parsed)) {
        errno = EINVAL; return nullptr;
    }
    if (!scriptFilesAvailable() || !scriptResourcesAvailable() || !scriptMemoryAvailable()) {
        errno = ENOSYS; return nullptr;
    }
    if (parsed.write && !scriptFileResizeAvailable()) { errno = ENOSYS; return nullptr; }
    size_t length = 0;
    while (length <= SHV_NX_FILE_MAX_PATH && path[length]) ++length;
    if (!length || length > SHV_NX_FILE_MAX_PATH) { errno = EINVAL; return nullptr; }
    uint64_t extent = 0;
    auto status = scriptFileSize(path, &extent);
    if (status != ShvNxFile_Ok && (status != ShvNxFile_NotFound || parsed.initial == 'r')) {
        errno = StatusError(status); return nullptr;
    }
    if (parsed.exclusive && status == ShvNxFile_Ok) { errno = EEXIST; return nullptr; }
    void* storage = scriptAllocate(sizeof(Stream), alignof(Stream), 0);
    if (!storage) { errno = ENOMEM; return nullptr; }
    auto* stream = ::new(storage) Stream{};
    for (size_t i = 0; i <= length; ++i) stream->path[i] = path[i];
    stream->extent = extent;
    stream->readable = parsed.read;
    stream->writable = parsed.write;
    stream->append = parsed.append;
    stream->resource = scriptResourceRegister(Close, stream);
    if (!stream->resource) { stream->~ShvNxReadStream(); (void)scriptFree(stream); errno = ENOMEM; return nullptr; }
    stream->next = first;
    if (first) first->previous = stream;
    first = stream;
    // Reserve all owner storage and cleanup before touching existing bytes.
    if (status == ShvNxFile_NotFound) status = scriptFileCreate(path, 0);
    else if (parsed.initial == 'w') status = scriptFileResize(path, 0);
    if (status != ShvNxFile_Ok) {
        (void)scriptResourceRelease(stream->resource);
        errno = StatusError(status); return nullptr;
    }
    if (parsed.initial == 'w') stream->extent = 0;
    if (parsed.append && !parsed.read) stream->position = stream->extent;
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
size_t fwrite(const void* input, size_t width, size_t count, FILE* pointer) {
    auto* stream = Find(pointer);
    if (!stream || !width || !count) return 0;
    if (!input) { Error(*stream, EINVAL); return 0; }
    if (count > SIZE_MAX / width) { Error(*stream, EOVERFLOW); return 0; }
    return Write(*stream, static_cast<const unsigned char*>(input), width * count) / width;
}
int fputc(int value, FILE* pointer) {
    auto* stream = Find(pointer);
    const unsigned char byte = static_cast<unsigned char>(value);
    return stream && Write(*stream, &byte, 1) == 1 ? int(byte) : EOF;
}
int putc(int value, FILE* stream) { return fputc(value, stream); }
int fputs(const char* text, FILE* pointer) {
    auto* stream = Find(pointer);
    if (!stream) return EOF;
    if (!text) { Error(*stream, EINVAL); return EOF; }
    size_t bytes = 0;
    while (bytes <= SHV_NX_FILE_MAX_BYTES && text[bytes]) ++bytes;
    if (bytes > SHV_NX_FILE_MAX_BYTES) { Error(*stream, EOVERFLOW); return EOF; }
    if (!stream->writable) { Error(*stream, EBADF); return EOF; }
    return !bytes || Write(*stream, reinterpret_cast<const unsigned char*>(text), bytes) == bytes ? 0 : EOF;
}
int fflush(FILE* pointer) {
    // Output is never buffered. Every successful write requests the existing
    // runtime flush option. No implicit publication or power-loss promise.
    if (pointer) return Find(pointer) ? 0 : EOF;
    ShvNxMemoryStats stats{};
    if (!scriptGetMemoryStats(&stats)) { errno = EPERM; return EOF; }
    return 0;
}
int vfprintf(FILE* pointer, const char* format, va_list arguments) {
    auto* stream = Find(pointer);
    if (!stream) return -1;
    if (!stream->writable) { Error(*stream, EBADF); return -1; }
    if (!format) { Error(*stream, EINVAL); return -1; }
    char local[512];
    va_list copy;
    va_copy(copy, arguments);
    const int length = vsnprintf(local, sizeof(local), format, copy);
    va_end(copy);
    if (length < 0) { Error(*stream, errno); return -1; }
    if (uint64_t(length) > SHV_NX_FILE_MAX_BYTES) { Error(*stream, EOVERFLOW); return -1; }
    if (size_t(length) < sizeof(local))
        return !length || Write(*stream, reinterpret_cast<const unsigned char*>(local), size_t(length)) == size_t(length) ? length : -1;
    auto* buffer = static_cast<char*>(scriptAllocate(size_t(length) + 1, 16, 0));
    if (!buffer) { Error(*stream, ENOMEM); return -1; }
    va_copy(copy, arguments);
    const int confirmed = vsnprintf(buffer, size_t(length) + 1, format, copy);
    va_end(copy);
    const bool ok = confirmed == length && Write(*stream, reinterpret_cast<const unsigned char*>(buffer), size_t(length)) == size_t(length);
    (void)scriptFree(buffer);
    if (confirmed != length) { Error(*stream, EINVAL); return -1; }
    return ok ? length : -1;
}
int fprintf(FILE* pointer, const char* format, ...) {
    va_list arguments;
    va_start(arguments, format);
    const int result = vfprintf(pointer, format, arguments);
    va_end(arguments);
    return result;
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
    if (!stream->readable) { Error(*stream, EBADF); return nullptr; }
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
