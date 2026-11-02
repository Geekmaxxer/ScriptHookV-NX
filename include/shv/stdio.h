#pragma once
#include <stdio.h>

// Opt-in, read-only freestanding streams. Only the documented read/seek/close
// functions are implemented; toolchain FILE declarations are opaque pointer types.
// The implementation does not create a Newlib FILE object. Do not access FILE
// internals or pass these pointers to another library's stdio implementation.
// No OS handle is retained across calls. See docs/reference/READ-STREAMS.md.

#ifdef __cplusplus
extern "C" {
#endif
FILE* fopen(const char* path, const char* mode);
int fclose(FILE* stream);
size_t fread(void* output, size_t element_bytes, size_t count, FILE* stream);
int fgetc(FILE* stream);
int getc(FILE* stream);
char* fgets(char* output, int capacity, FILE* stream);
int fseek(FILE* stream, long offset, int origin);
long ftell(FILE* stream);
void rewind(FILE* stream);
int feof(FILE* stream);
int ferror(FILE* stream);
void clearerr(FILE* stream);
#ifdef __cplusplus
}
#endif
